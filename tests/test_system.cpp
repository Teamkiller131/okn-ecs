#include <doctest/doctest.h>
#include <okn/ecs/scheduler/system.hpp>
#include <okn/ecs/world.hpp>

namespace okn::ecs {
namespace {

struct Position {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Velocity {
    float dx = 0.0f;
    float dy = 0.0f;
    float dz = 0.0f;
};

class MovementSystem : public System {
public:
    MovementSystem() { set_name("MovementSystem"); }

    auto reads() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Velocity>()};
    }
    auto writes() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Position>()};
    }
    auto before() const -> std::vector<std::string> override {
        return {"RenderSystem"};
    }
    auto after() const -> std::vector<std::string> override {
        return {"InputSystem"};
    }

    void execute(World& world, float delta_time) override {
        execute_count++;
        auto view = world.query<Position, Velocity>();
        for (auto [entity, pos, vel] : view) {
            (void)entity;
            pos->x += vel->dx * delta_time;
            pos->y += vel->dy * delta_time;
            pos->z += vel->dz * delta_time;
        }
    }

    int execute_count = 0;
};

class InputSystem : public System {
public:
    InputSystem() { set_name("InputSystem"); }

    auto writes() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Velocity>()};
    }

    void execute(World& world, float) override {
        execute_count++;
        auto view = world.query<Velocity>();
        for (auto [entity, vel] : view) {
            (void)entity;
            vel->dx = 1.0f;
            vel->dy = 0.0f;
            vel->dz = 0.0f;
        }
    }

    int execute_count = 0;
};

} // namespace
} // namespace okn::ecs

using namespace okn::ecs;

TEST_CASE("System - basic properties") {
    SUBCASE("default name is empty") {
        class EmptySystem : public System {
        public:
            void execute(World&, float) override {}
        };
        EmptySystem sys;
        CHECK(sys.name().empty());
    }

    SUBCASE("set and get name") {
        class NamedSystem : public System {
        public:
            void execute(World&, float) override {}
        };
        NamedSystem sys;
        sys.set_name("TestSystem");
        CHECK(sys.name() == "TestSystem");
    }

    SUBCASE("default reads/writes are empty") {
        class NoDepsSystem : public System {
        public:
            void execute(World&, float) override {}
        };
        NoDepsSystem sys;
        CHECK(sys.reads().empty());
        CHECK(sys.writes().empty());
    }

    SUBCASE("default before/after are empty") {
        class NoOrderSystem : public System {
        public:
            void execute(World&, float) override {}
        };
        NoOrderSystem sys;
        CHECK(sys.before().empty());
        CHECK(sys.after().empty());
    }
}

TEST_CASE("System - declared reads/writes") {
    MovementSystem sys;
    auto r = sys.reads();
    auto w = sys.writes();
    CHECK(r.size() == 1);
    CHECK(w.size() == 1);
    CHECK(r[0] == World::component_type_id<Velocity>());
    CHECK(w[0] == World::component_type_id<Position>());
}

TEST_CASE("System - declared ordering") {
    MovementSystem sys;
    auto b = sys.before();
    auto a = sys.after();
    CHECK(b.size() == 1);
    CHECK(a.size() == 1);
    CHECK(b[0] == "RenderSystem");
    CHECK(a[0] == "InputSystem");
}

TEST_CASE("System - execute with world") {
    World world;
    auto e = world.create_entity();
    world.add_component(e, Position{0.0f, 0.0f, 0.0f});
    world.add_component(e, Velocity{10.0f, 5.0f, 0.0f});

    MovementSystem sys;

    SUBCASE("execute applies velocity to position") {
        sys.execute(world, 1.0f);
        auto* pos = world.get_component<Position>(e);
        REQUIRE(pos != nullptr);
        CHECK(pos->x == 10.0f);
        CHECK(pos->y == 5.0f);
        CHECK(pos->z == 0.0f);
        CHECK(sys.execute_count == 1);
    }

    SUBCASE("execute with delta time") {
        sys.execute(world, 0.5f);
        auto* pos = world.get_component<Position>(e);
        CHECK(pos->x == 5.0f);
        CHECK(pos->y == 2.5f);
    }
}
