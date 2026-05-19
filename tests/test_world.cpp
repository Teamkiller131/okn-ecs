#include <doctest/doctest.h>
#include <okn/ecs/world.hpp>
#include <okn/ecs/world_builder.hpp>

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

struct NameComponent {
    u32 hash = 0;
};

} // namespace
} // namespace okn::ecs

using namespace okn::ecs;

TEST_CASE("World - create and destroy entities") {
    World world;

    SUBCASE("create entity returns valid entity") {
        auto e = world.create_entity();
        CHECK(e.is_valid());
        CHECK(world.entity_alive(e));
        CHECK(world.entity_count() == 1);
    }

    SUBCASE("create multiple entities") {
        auto e1 = world.create_entity();
        auto e2 = world.create_entity();
        auto e3 = world.create_entity();
        CHECK(world.entity_count() == 3);
        CHECK(e1 != e2);
        CHECK(e2 != e3);
        CHECK(e1 != e3);
    }

    SUBCASE("destroy entity") {
        auto e = world.create_entity();
        world.destroy_entity(e);
        CHECK(!world.entity_alive(e));
        CHECK(world.entity_count() == 0);
    }

    SUBCASE("reuse entity index") {
        auto e1 = world.create_entity();
        world.destroy_entity(e1);
        auto e2 = world.create_entity();
        CHECK(e1.index() == e2.index());
        CHECK(e1.generation() != e2.generation());
        CHECK(world.entity_alive(e2));
        CHECK(!world.entity_alive(e1));
    }

    SUBCASE("destroy already dead entity is safe") {
        auto e = world.create_entity();
        world.destroy_entity(e);
        world.destroy_entity(e);
        CHECK(world.entity_count() == 0);
    }

    SUBCASE("invalid entity is not alive") {
        CHECK(!world.entity_alive(Entity{}));
    }
}

TEST_CASE("World - component management") {
    World world;

    SUBCASE("add and get component") {
        auto e = world.create_entity();
        Position pos{1.0f, 2.0f, 3.0f};
        world.add_component(e, pos);
        CHECK(world.has_component<Position>(e));

        auto* got = world.get_component<Position>(e);
        REQUIRE(got != nullptr);
        CHECK(got->x == 1.0f);
        CHECK(got->y == 2.0f);
        CHECK(got->z == 3.0f);
    }

    SUBCASE("overwrite component") {
        auto e = world.create_entity();
        world.add_component(e, Position{1.0f, 0.0f, 0.0f});
        world.add_component(e, Position{5.0f, 0.0f, 0.0f});
        auto* got = world.get_component<Position>(e);
        CHECK(got->x == 5.0f);
    }

    SUBCASE("remove component") {
        auto e = world.create_entity();
        world.add_component(e, Position{1.0f, 2.0f, 3.0f});
        world.remove_component<Position>(e);
        CHECK(!world.has_component<Position>(e));
        CHECK(world.get_component<Position>(e) == nullptr);
    }

    SUBCASE("remove non-existent component is safe") {
        auto e = world.create_entity();
        world.remove_component<Position>(e);
        CHECK(!world.has_component<Position>(e));
    }

    SUBCASE("get component from entity without it") {
        auto e = world.create_entity();
        CHECK(world.get_component<Position>(e) == nullptr);
    }

    SUBCASE("multiple component types") {
        auto e = world.create_entity();
        world.add_component(e, Position{1.0f, 2.0f, 3.0f});
        world.add_component(e, Velocity{0.1f, 0.2f, 0.3f});

        CHECK(world.has_component<Position>(e));
        CHECK(world.has_component<Velocity>(e));
        CHECK(!world.has_component<NameComponent>(e));

        auto* pos = world.get_component<Position>(e);
        auto* vel = world.get_component<Velocity>(e);
        CHECK(pos->x == 1.0f);
        CHECK(vel->dx == 0.1f);
    }

    SUBCASE("removing one component does not affect others") {
        auto e = world.create_entity();
        world.add_component(e, Position{1.0f, 2.0f, 3.0f});
        world.add_component(e, Velocity{0.1f, 0.2f, 0.3f});

        world.remove_component<Velocity>(e);

        CHECK(world.has_component<Position>(e));
        CHECK(!world.has_component<Velocity>(e));
    }

    SUBCASE("destroy entity removes all components") {
        auto e = world.create_entity();
        world.add_component(e, Position{1.0f, 2.0f, 3.0f});
        world.add_component(e, Velocity{0.1f, 0.2f, 0.3f});

        world.destroy_entity(e);

        CHECK(!world.entity_alive(e));
        CHECK(world.get_component<Position>(e) == nullptr);
        CHECK(world.get_component<Velocity>(e) == nullptr);
    }

    SUBCASE("const get_component") {
        auto e = world.create_entity();
        world.add_component(e, Position{1.0f, 2.0f, 3.0f});

        const World& cworld = world;
        const Position* got = cworld.get_component<Position>(e);
        REQUIRE(got != nullptr);
        CHECK(got->x == 1.0f);
    }
}

TEST_CASE("World - query") {
    World world;

    SUBCASE("empty query returns empty view") {
        auto view = world.query<Position>();
        CHECK(view.empty());
        CHECK(view.size() == 0);
    }

    SUBCASE("single component query") {
        auto e1 = world.create_entity();
        auto e2 = world.create_entity();
        auto e3 = world.create_entity();

        world.add_component(e1, Position{1.0f, 0.0f, 0.0f});
        world.add_component(e2, Position{2.0f, 0.0f, 0.0f});

        auto view = world.query<Position>();
        CHECK(view.size() == 2);
        CHECK(!view.empty());
    }

    SUBCASE("two component query") {
        auto e1 = world.create_entity();
        auto e2 = world.create_entity();
        auto e3 = world.create_entity();

        world.add_component(e1, Position{1.0f, 0.0f, 0.0f});
        world.add_component(e1, Velocity{0.1f, 0.0f, 0.0f});
        world.add_component(e2, Position{2.0f, 0.0f, 0.0f});
        world.add_component(e3, Velocity{0.3f, 0.0f, 0.0f});

        auto view = world.query<Position, Velocity>();
        CHECK(view.size() == 1);

        for (auto [entity, pos, vel] : view) {
            (void)entity;
            CHECK(pos->x == 1.0f);
            CHECK(vel->dx == 0.1f);
        }
    }

    SUBCASE("query iteration modifies components") {
        auto e1 = world.create_entity();
        auto e2 = world.create_entity();
        world.add_component(e1, Position{1.0f, 2.0f, 3.0f});
        world.add_component(e2, Position{4.0f, 5.0f, 6.0f});

        auto view = world.query<Position>();
        for (auto [entity, pos] : view) {
            (void)entity;
            pos->x += 10.0f;
        }

        CHECK(world.get_component<Position>(e1)->x == 11.0f);
        CHECK(world.get_component<Position>(e2)->x == 14.0f);
    }
}

TEST_CASE("WorldBuilder") {
    WorldBuilder builder;
    builder.register_component<Position>();
    builder.register_component<Velocity>();
    auto world = builder.build();

    auto e = world.create_entity();
    world.add_component(e, Position{1.0f, 2.0f, 3.0f});
    world.add_component(e, Velocity{0.1f, 0.2f, 0.3f});

    CHECK(world.has_component<Position>(e));
    CHECK(world.has_component<Velocity>(e));
}

TEST_CASE("World - move semantics") {
    World world1;
    auto e = world1.create_entity();
    world1.add_component(e, Position{1.0f, 2.0f, 3.0f});

    World world2 = std::move(world1);

    CHECK(world2.entity_alive(e));
    auto* pos = world2.get_component<Position>(e);
    REQUIRE(pos != nullptr);
    CHECK(pos->x == 1.0f);
    CHECK(world2.entity_count() == 1);
}
