#include <doctest/doctest.h>
#include <okn/ecs/scheduler/scheduler.hpp>
#include <okn/ecs/scheduler/system_graph.hpp>
#include <okn/ecs/scheduler/job_adapter.hpp>
#include <okn/ecs/world.hpp>

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
