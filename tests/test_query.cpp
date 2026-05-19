#include <doctest/doctest.h>
#include <okn/ecs/world.hpp>
#include <okn/ecs/query/query.hpp>
#include <okn/ecs/query/filter.hpp>
#include <okn/ecs/query/relation.hpp>

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

struct Health {
    int value = 100;
};

struct Tag {};

} // namespace
} // namespace okn::ecs

using namespace okn::ecs;

TEST_CASE("Filter - matches") {
    Filter filter;

    SUBCASE("empty filter matches everything") {
        CHECK(filter.matches({}));
        CHECK(filter.matches({World::component_type_id<Position>()}));
    }

    SUBCASE("with filter") {
        auto pos_id = World::component_type_id<Position>();
        auto vel_id = World::component_type_id<Velocity>();
        filter.with.push_back(pos_id);

        CHECK(filter.matches({pos_id}));
        CHECK(filter.matches({pos_id, vel_id}));
        CHECK(!filter.matches({vel_id}));
        CHECK(!filter.matches({}));
    }

    SUBCASE("without filter") {
        auto pos_id = World::component_type_id<Position>();
        filter.without.push_back(pos_id);

        CHECK(!filter.matches({pos_id}));
        CHECK(filter.matches({}));
        CHECK(filter.matches({World::component_type_id<Velocity>()}));
    }

    SUBCASE("with and without") {
        auto pos_id = World::component_type_id<Position>();
        auto vel_id = World::component_type_id<Velocity>();
        filter.with.push_back(pos_id);
        filter.without.push_back(vel_id);

        CHECK(filter.matches({pos_id}));
        CHECK(!filter.matches({pos_id, vel_id}));
        CHECK(!filter.matches({vel_id}));
        CHECK(!filter.matches({}));
    }
}

TEST_CASE("Query - filter evaluation") {
    World world;

    SUBCASE("query with filter - with condition") {
        auto pos_id = World::component_type_id<Position>();
        auto e1 = world.create_entity();
        auto e2 = world.create_entity();
        auto e3 = world.create_entity();

        world.add_component(e1, Position{1.0f, 0.0f, 0.0f});
        world.add_component(e2, Position{2.0f, 0.0f, 0.0f});
        world.add_component(e3, Velocity{0.1f, 0.0f, 0.0f});

        Filter filter;
        filter.with.push_back(pos_id);

        Query query(world, filter);
        CHECK(query.entity_count() == 2);

        for (auto e : query.entities()) {
            CHECK(world.has_component<Position>(e));
        }
    }

    SUBCASE("query with filter - multiple with") {
        auto pos_id = World::component_type_id<Position>();
        auto vel_id = World::component_type_id<Velocity>();

        auto e1 = world.create_entity();
        auto e2 = world.create_entity();

        world.add_component(e1, Position{1.0f, 0.0f, 0.0f});
        world.add_component(e1, Velocity{0.1f, 0.0f, 0.0f});
        world.add_component(e2, Position{2.0f, 0.0f, 0.0f});

        Filter filter;
        filter.with.push_back(pos_id);
        filter.with.push_back(vel_id);

        Query query(world, filter);
        CHECK(query.entity_count() == 1);
    }

    SUBCASE("query with filter - without condition") {
        auto vel_id = World::component_type_id<Velocity>();

        auto e1 = world.create_entity();
        auto e2 = world.create_entity();

        world.add_component(e1, Position{1.0f, 0.0f, 0.0f});
        world.add_component(e2, Position{2.0f, 0.0f, 0.0f});
        world.add_component(e2, Velocity{0.1f, 0.0f, 0.0f});

        Filter filter;
        filter.without.push_back(vel_id);

        Query query(world, filter);
        CHECK(query.entity_count() == 1);
    }

    SUBCASE("query empty world") {
        Filter filter;
        Query query(world, filter);
        CHECK(query.entity_count() == 0);
    }
}

TEST_CASE("Relation - parent child") {
    World world;
    Relation relation;

    SUBCASE("set and get parent") {
        auto parent = world.create_entity();
        auto child = world.create_entity();

        relation.set_parent(world, child, parent);

        CHECK(relation.get_parent(child).index() == parent.index());
        CHECK(!relation.get_parent(parent).is_valid());
    }

    SUBCASE("get children") {
        auto parent = world.create_entity();
        auto child1 = world.create_entity();
        auto child2 = world.create_entity();

        relation.set_parent(world, child1, parent);
        relation.set_parent(world, child2, parent);

        auto children = relation.get_children(parent);
        CHECK(children.size() == 2);
    }

    SUBCASE("remove parent") {
        auto parent = world.create_entity();
        auto child = world.create_entity();

        relation.set_parent(world, child, parent);
        relation.remove_parent(child);

        CHECK(!relation.get_parent(child).is_valid());
        CHECK(relation.get_children(parent).empty());
    }

    SUBCASE("reparent") {
        auto parent1 = world.create_entity();
        auto parent2 = world.create_entity();
        auto child = world.create_entity();

        relation.set_parent(world, child, parent1);
        relation.set_parent(world, child, parent2);

        CHECK(relation.get_parent(child).index() == parent2.index());
        CHECK(relation.get_children(parent1).empty());
        CHECK(relation.get_children(parent2).size() == 1);
    }
}

TEST_CASE("View - iteration") {
    World world;

    SUBCASE("view iteration with single component") {
        auto e1 = world.create_entity();
        auto e2 = world.create_entity();
        world.add_component(e1, Position{1.0f, 2.0f, 3.0f});
        world.add_component(e2, Position{4.0f, 5.0f, 6.0f});

        auto view = world.query<Position>();
        int count = 0;
        for (auto [entity, pos] : view) {
            (void)entity;
            CHECK(pos != nullptr);
            ++count;
        }
        CHECK(count == 2);
    }

    SUBCASE("view iteration with two components") {
        auto e1 = world.create_entity();
        auto e2 = world.create_entity();
        auto e3 = world.create_entity();

        world.add_component(e1, Position{1.0f, 0.0f, 0.0f});
        world.add_component(e1, Velocity{0.1f, 0.0f, 0.0f});
        world.add_component(e2, Position{2.0f, 0.0f, 0.0f});
        world.add_component(e3, Velocity{0.3f, 0.0f, 0.0f});

        auto view = world.query<Position, Velocity>();
        int count = 0;
        for (auto [entity, pos, vel] : view) {
            (void)entity;
            CHECK(pos != nullptr);
            CHECK(vel != nullptr);
            ++count;
        }
        CHECK(count == 1);
    }

    SUBCASE("empty view") {
        auto view = world.query<Position>();
        int count = 0;
        for (auto [entity, pos] : view) {
            (void)entity;
            (void)pos;
            ++count;
        }
        CHECK(count == 0);
    }

    SUBCASE("view with zero component types") {
        world.create_entity();
        world.create_entity();
        auto view = world.query<>();
        CHECK(view.size() == 0);
    }
}

TEST_CASE("View - structured binding with three components") {
    World world;
    auto e = world.create_entity();
    world.add_component(e, Position{1.0f, 2.0f, 3.0f});
    world.add_component(e, Velocity{0.1f, 0.2f, 0.3f});
    world.add_component(e, Health{75});

    auto view = world.query<Position, Velocity, Health>();
    CHECK(view.size() == 1);

    for (auto [entity, pos, vel, health] : view) {
        (void)entity;
        CHECK(pos->x == 1.0f);
        CHECK(pos->y == 2.0f);
        CHECK(pos->z == 3.0f);
        CHECK(vel->dx == 0.1f);
        CHECK(vel->dy == 0.2f);
        CHECK(vel->dz == 0.3f);
        CHECK(health->value == 75);
    }
}
