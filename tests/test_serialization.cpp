#include <doctest/doctest.h>

#include <okn/ecs/world.hpp>
#include <okn/ecs/serialization/serialize.hpp>
#include <okn/ecs/serialization/entity_refs.hpp>

#include <cstddef>
#include <cstdio>
#include <vector>

namespace okn::ecs {
namespace {

struct Position { float x, y, z; };
struct Velocity { float dx, dy; };
struct Health { int hp; };
struct Link { Entity target; int tag; };   // holds a cross-entity reference

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

TEST_CASE("okn::ecs::Serializer - remaps entity references across save/load") {
    register_entity_ref_fields<Link>({static_cast<u32>(offsetof(Link, target))});

    World a;
    // Churn ids so the saved entities carry generation>1 — a reloaded world always
    // assigns fresh (index, gen=1) ids, so an UN-remapped ref would keep a stale id
    // that no longer resolves. This makes the remap observable, not coincidental.
    Entity f0 = a.create_entity();
    Entity f1 = a.create_entity();
    a.destroy_entity(f0);
    a.destroy_entity(f1);
    Entity hero = a.create_entity();    // reuses a freed index at generation 2
    Entity sword = a.create_entity();   // reuses the other freed index at generation 2
    REQUIRE(hero.generation() >= 2);
    REQUIRE(sword.generation() >= 2);

    a.add_component<Link>(hero, {sword, 42});   // hero references sword
    a.add_component<Health>(sword, {7});

    Serializer ser(a);
    const std::vector<u8> bytes = ser.save();

    World b;
    Serializer loader(b);
    REQUIRE(loader.load(bytes));
    REQUIRE(b.entity_count() == 2);

    // Identify the reloaded entities by which component they carry.
    Entity loaded_hero{}, loaded_sword{};
    for (auto [e, l] : b.query<Link>()) { (void)l; loaded_hero = e; }
    for (auto [e, h] : b.query<Health>()) { (void)h; loaded_sword = e; }
    REQUIRE(loaded_hero.is_valid());
    REQUIRE(loaded_sword.is_valid());

    const Link* link = b.get_component<Link>(loaded_hero);
    REQUIRE(link != nullptr);
    CHECK(link->tag == 42);
    // The reference was remapped to the NEW sword id, not the stale saved one.
    CHECK(link->target == loaded_sword);
    CHECK(b.entity_alive(link->target));
}

TEST_CASE("okn::ecs::Serializer - save_to_file/load_from_file survives a restart with refs") {
    register_entity_ref_fields<Link>({static_cast<u32>(offsetof(Link, target))});
    const char* path = "okn_ecs_scene_roundtrip.eko";
    std::remove(path);

    // "Save the scene": churn ids to gen>1, link hero->sword, write to disk.
    {
        World a;
        Entity f0 = a.create_entity();
        Entity f1 = a.create_entity();
        a.destroy_entity(f0);
        a.destroy_entity(f1);
        Entity hero = a.create_entity();
        Entity sword = a.create_entity();
        a.add_component<Link>(hero, {sword, 7});
        a.add_component<Health>(sword, {33});
        Serializer ser(a);
        REQUIRE(ser.save_to_file(path));
    }

    // "Restart": a brand-new World loads the file from scratch.
    World b;
    Serializer loader(b);
    REQUIRE(loader.load_from_file(path));
    REQUIRE(b.entity_count() == 2);

    Entity loaded_hero{}, loaded_sword{};
    for (auto [e, l] : b.query<Link>()) { (void)l; loaded_hero = e; }
    for (auto [e, h] : b.query<Health>()) { (void)h; loaded_sword = e; }
    REQUIRE(loaded_hero.is_valid());
    REQUIRE(loaded_sword.is_valid());

    const Link* link = b.get_component<Link>(loaded_hero);
    REQUIRE(link != nullptr);
    CHECK(link->tag == 7);
    CHECK(link->target == loaded_sword);   // ref survived the disk round-trip, remapped
    CHECK(b.entity_alive(link->target));

    // A missing file fails gracefully (no throw, no partial load).
    World c;
    Serializer cl(c);
    CHECK_FALSE(cl.load_from_file("okn_ecs_no_such_scene.eko"));
    CHECK(c.entity_count() == 0);

    std::remove(path);
}

} // namespace okn::ecs
