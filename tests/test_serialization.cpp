#include <doctest/doctest.h>

#include <okn/ecs/world.hpp>
#include <okn/ecs/serialization/serialize.hpp>

#include <vector>

namespace okn::ecs {
namespace {

struct Position { float x, y, z; };
struct Velocity { float dx, dy; };
struct Health { int hp; };

} // namespace

TEST_CASE("okn::ecs::Serializer - round-trips entities and components") {
    World a;

    Entity e1 = a.create_entity();
    a.add_component<Position>(e1, {1.0f, 2.0f, 3.0f});
    a.add_component<Velocity>(e1, {4.0f, 5.0f});

    Entity e2 = a.create_entity();
    a.add_component<Position>(e2, {10.0f, 20.0f, 30.0f});

    Entity e3 = a.create_entity();
    a.add_component<Health>(e3, {100});

    Serializer ser(a);
    const std::vector<u8> bytes = ser.save();
    CHECK(!bytes.empty());

    World b;
    Serializer loader(b);
    CHECK(loader.load(bytes));

    CHECK(b.entity_count() == 3);

    // Position lives on two entities with the original values.
    int pos_count = 0;
    bool saw_1 = false;
    bool saw_10 = false;
    for (auto [e, p] : b.query<Position>()) {
        (void)e;
        ++pos_count;
        if (p->x == doctest::Approx(1.0f)) {
            saw_1 = true;
            CHECK(p->y == doctest::Approx(2.0f));
            CHECK(p->z == doctest::Approx(3.0f));
        }
        if (p->x == doctest::Approx(10.0f)) {
            saw_10 = true;
            CHECK(p->y == doctest::Approx(20.0f));
            CHECK(p->z == doctest::Approx(30.0f));
        }
    }
    CHECK(pos_count == 2);
    CHECK(saw_1);
    CHECK(saw_10);

    // Velocity only on the first entity.
    int vel_count = 0;
    for (auto [e, v] : b.query<Velocity>()) {
        (void)e;
        ++vel_count;
        CHECK(v->dx == doctest::Approx(4.0f));
        CHECK(v->dy == doctest::Approx(5.0f));
    }
    CHECK(vel_count == 1);

    // Health only on the third entity.
    int hp_count = 0;
    for (auto [e, h] : b.query<Health>()) {
        (void)e;
        ++hp_count;
        CHECK(h->hp == 100);
    }
    CHECK(hp_count == 1);

    // The Position+Velocity entity carries both, intact.
    int both = 0;
    for (auto [e, p, v] : b.query<Position, Velocity>()) {
        (void)e;
        ++both;
        CHECK(p->x == doctest::Approx(1.0f));
        CHECK(v->dy == doctest::Approx(5.0f));
    }
    CHECK(both == 1);
}

TEST_CASE("okn::ecs::Serializer - empty world round-trips to empty") {
    World a;
    Serializer ser(a);
    const auto bytes = ser.save();

    World b;
    Serializer loader(b);
    CHECK(loader.load(bytes));
    CHECK(b.entity_count() == 0);
}

TEST_CASE("okn::ecs::Serializer - rejects corrupt data") {
    World b;
    Serializer loader(b);
    CHECK_FALSE(loader.load({}));                        // too short for the magic
    CHECK_FALSE(loader.load({0, 1, 2, 3, 4, 5, 6, 7}));  // wrong magic
    CHECK(b.entity_count() == 0);
}

TEST_CASE("okn::ecs::Serializer - destroyed entities are not saved") {
    World a;
    Entity keep = a.create_entity();
    a.add_component<Health>(keep, {7});
    Entity gone = a.create_entity();
    a.add_component<Health>(gone, {99});
    a.destroy_entity(gone);

    Serializer ser(a);
    const auto bytes = ser.save();

    World b;
    Serializer loader(b);
    CHECK(loader.load(bytes));
    CHECK(b.entity_count() == 1);

    int n = 0;
    for (auto [e, h] : b.query<Health>()) {
        (void)e;
        ++n;
        CHECK(h->hp == 7);
    }
    CHECK(n == 1);
}

} // namespace okn::ecs
