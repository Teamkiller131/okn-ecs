#include <doctest/doctest.h>
#include <okn/ecs/scheduler/scheduler.hpp>
#include <okn/ecs/scheduler/system_graph.hpp>
#include <okn/ecs/scheduler/job_adapter.hpp>
#include <okn/ecs/scheduler/thread_pool_job_system.hpp>
#include <okn/ecs/world.hpp>

#include <atomic>

namespace okn::ecs {
namespace {

struct Position { float x = 0; float y = 0; float z = 0; };
struct Velocity { float dx = 0; float dy = 0; float dz = 0; };
struct Health   { int hp = 100; };

class MoveSystem : public System {
public:
    MoveSystem() { set_name("MoveSystem"); }
    auto reads() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Velocity>()};
    }
    auto writes() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Position>()};
    }
    void execute(World& world, float dt) override {
        exec_count++;
        auto view = world.query<Position, Velocity>();
        for (auto [entity, pos, vel] : view) {
            (void)entity;
            pos->x += vel->dx * dt;
            pos->y += vel->dy * dt;
            pos->z += vel->dz * dt;
        }
    }
    int exec_count = 0;
};

class DamageSystem : public System {
public:
    DamageSystem() { set_name("DamageSystem"); }
    auto reads() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Position>()};
    }
    auto writes() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Health>()};
    }
    auto after() const -> std::vector<std::string> override {
        return {"MoveSystem"};
    }
    void execute(World& world, float) override {
        exec_count++;
        auto view = world.query<Position, Health>();
        for (auto [entity, pos, hp] : view) {
            (void)entity;
            (void)pos;
            hp->hp -= 1;
        }
    }
    int exec_count = 0;
};

// Two systems with DISJOINT writes and no shared reads: the SystemGraph places
// them in the same parallel level, so the Scheduler dispatches them concurrently
// through the job system.
struct CompA { int v = 0; };
struct CompB { int v = 0; };

class SysA : public System {
public:
    SysA() { set_name("SysA"); }
    auto writes() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<CompA>()};
    }
    void execute(World& world, float) override {
        runs.fetch_add(1, std::memory_order_relaxed);
        for (auto [e, a] : world.query<CompA>()) { (void)e; a->v += 1; }
    }
    std::atomic<int> runs{0};
};

class SysB : public System {
public:
    SysB() { set_name("SysB"); }
    auto writes() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<CompB>()};
    }
    void execute(World& world, float) override {
        runs.fetch_add(1, std::memory_order_relaxed);
        for (auto [e, b] : world.query<CompB>()) { (void)e; b->v += 1; }
    }
    std::atomic<int> runs{0};
};

} // namespace
} // namespace okn::ecs

using namespace okn::ecs;

TEST_CASE("Scheduler - basic execution") {
    SystemGraph graph;
    graph.add_system(std::make_unique<MoveSystem>());
    graph.add_system(std::make_unique<DamageSystem>());

    Scheduler scheduler(graph);

    World world;
    auto e = world.create_entity();
    world.add_component(e, Position{0.0f, 0.0f, 0.0f});
    world.add_component(e, Velocity{1.0f, 2.0f, 3.0f});
    world.add_component(e, Health{100});

    scheduler.run(world, 0.5f);

    auto* pos = world.get_component<Position>(e);
    auto* hp = world.get_component<Health>(e);
    REQUIRE(pos != nullptr);
    REQUIRE(hp != nullptr);
    CHECK(pos->x == 0.5f);
    CHECK(pos->y == 1.0f);
    CHECK(pos->z == 1.5f);
    CHECK(hp->hp == 99);
}

TEST_CASE("Scheduler - invalidate order") {
    SystemGraph graph;
    graph.add_system(std::make_unique<MoveSystem>());
    Scheduler scheduler(graph);

    scheduler.invalidate_order();

    World world;
    auto e = world.create_entity();
    world.add_component(e, Position{0.0f, 0.0f, 0.0f});
    world.add_component(e, Velocity{2.0f, 0.0f, 0.0f});

    scheduler.run(world, 1.0f);

    auto* pos = world.get_component<Position>(e);
    CHECK(pos->x == 2.0f);
}

TEST_CASE("Scheduler - empty graph") {
    SystemGraph graph;
    Scheduler scheduler(graph);

    World world;
    scheduler.run(world, 1.0f);
}

TEST_CASE("Scheduler - job system") {
    IJobSystem* js = nullptr;
    SystemGraph graph;
    graph.add_system(std::make_unique<MoveSystem>());
    Scheduler scheduler(graph);
    scheduler.set_job_system(js);

    World world;
    auto e = world.create_entity();
    world.add_component(e, Position{0.0f, 0.0f, 0.0f});
    world.add_component(e, Velocity{1.0f, 0.0f, 0.0f});

    scheduler.run(world, 1.0f);
    auto* pos = world.get_component<Position>(e);
    CHECK(pos->x == 1.0f);
}

TEST_CASE("Scheduler - parallel path runs systems via a real thread pool") {
    SystemGraph graph;
    auto a = std::make_unique<SysA>();
    auto b = std::make_unique<SysB>();
    auto* pa = a.get();
    auto* pb = b.get();
    graph.add_system(std::move(a));
    graph.add_system(std::move(b));

    Scheduler scheduler(graph);
    ThreadPoolJobSystem pool(4);
    scheduler.set_job_system(&pool);  // exercises run_parallel() with real worker threads

    World world;
    constexpr int kEntities = 1000;
    std::vector<Entity> ents;
    ents.reserve(kEntities);
    for (int i = 0; i < kEntities; ++i) {
        auto e = world.create_entity();
        world.add_component(e, CompA{0});
        world.add_component(e, CompB{0});
        ents.push_back(e);
    }

    constexpr int kFrames = 5;
    for (int f = 0; f < kFrames; ++f) {
        scheduler.run(world, 1.0f);
    }

    // Both systems ran every frame, and each (concurrently-dispatched) system
    // advanced every entity exactly once per frame.
    CHECK(pa->runs.load() == kFrames);
    CHECK(pb->runs.load() == kFrames);
    for (auto e : ents) {
        CHECK(world.get_component<CompA>(e)->v == kFrames);
        CHECK(world.get_component<CompB>(e)->v == kFrames);
    }
    CHECK(pool.worker_count() >= 1);
}

TEST_CASE("Scheduler - conflict levels are built once and cached across frames") {
    SystemGraph graph;
    graph.add_system(std::make_unique<SysA>());
    graph.add_system(std::make_unique<SysB>());

    Scheduler scheduler(graph);
    ThreadPoolJobSystem pool(2);
    scheduler.set_job_system(&pool);

    World world;
    auto e = world.create_entity();
    world.add_component(e, CompA{0});
    world.add_component(e, CompB{0});

    // SysA writes CompA, SysB writes CompB (disjoint, no shared reads) -> one level.
    constexpr int kFrames = 8;
    for (int f = 0; f < kFrames; ++f) {
        scheduler.run(world, 1.0f);
    }
    CHECK(scheduler.level_count() == 1);            // the two systems run concurrently
    CHECK(scheduler.levelization_count() == 1);     // built once, reused all 8 frames

    // Changing the system set invalidates the cache; the next run rebuilds it once.
    scheduler.invalidate_order();
    scheduler.run(world, 1.0f);
    CHECK(scheduler.levelization_count() == 2);

    // ...and then it's stable again across further frames.
    scheduler.run(world, 1.0f);
    scheduler.run(world, 1.0f);
    CHECK(scheduler.levelization_count() == 2);

    CHECK(world.get_component<CompA>(e)->v == kFrames + 3);   // behavior intact
}
