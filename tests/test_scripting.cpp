#include <doctest/doctest.h>

#include <okn/ecs/scripting/scripting_bridge.hpp>
#include <okn/ecs/world.hpp>

#include <string>
#include <vector>

using namespace okn::ecs;

namespace {
struct Position { float x, y; };
struct Velocity { float dx, dy; };
}  // namespace

TEST_CASE("okn::ecs::ScriptingBridge - entity create/destroy via script API") {
    World w;
    auto e = ScriptingBridge::script_create_entity(w);
    CHECK(w.entity_alive(e));
    ScriptingBridge::script_destroy_entity(w, e);
    CHECK_FALSE(w.entity_alive(e));
    // destroying an already-dead entity is a safe no-op
    ScriptingBridge::script_destroy_entity(w, e);
    CHECK_FALSE(w.entity_alive(e));
}

TEST_CASE("okn::ecs::World - has_component_by_id matches templated has_component") {
    World w;
    auto e = w.create_entity();
    w.add_component<Position>(e, {3.0f, 4.0f});

    CHECK(w.has_component<Position>(e));
    CHECK(w.has_component_by_id(e, World::component_type_id<Position>()));
    CHECK_FALSE(w.has_component_by_id(e, World::component_type_id<Velocity>()));
    CHECK_FALSE(w.has_component_by_id(e, 0));  // unknown id
}

TEST_CASE("okn::ecs::ScriptingBridge - has_component resolves names to live storage") {
    World w;
    ScriptingBridge bridge(w);
    bridge.register_component<Position>("Position");
    bridge.register_component<Velocity>("Velocity");
    CHECK(bridge.registered_count() == 2);

    // the bridge's id is the SAME id the World stores under
    CHECK(bridge.component_id("Position") == World::component_type_id<Position>());
    CHECK(bridge.component_id("Unknown") == 0);

    auto e = w.create_entity();
    w.add_component<Position>(e, {1.0f, 2.0f});

    CHECK(bridge.has_component(e, "Position"));        // attached
    CHECK_FALSE(bridge.has_component(e, "Velocity"));  // registered, not attached
    CHECK_FALSE(bridge.has_component(e, "Unknown"));   // never registered
    CHECK_FALSE(bridge.has_component(e, nullptr));     // null-safe
}

TEST_CASE("okn::ecs::ScriptingBridge - register_all_to_script forwards each component") {
    World w;
    ScriptingBridge bridge(w);
    bridge.register_component<Position>("Position");
    bridge.register_component<Velocity>("Velocity");

    struct Sink { std::vector<std::string> names; std::vector<ComponentTypeId> ids; };
    Sink sink;
    auto fn = [](void* ctx, const char* name, ComponentTypeId id, usize size) {
        auto* s = static_cast<Sink*>(ctx);
        s->names.emplace_back(name);
        s->ids.push_back(id);
        CHECK(size > 0);
    };

    const auto n = bridge.register_all_to_script(&sink, fn);
    CHECK(n == 2);
    CHECK(sink.names.size() == 2);
    CHECK(sink.ids.size() == 2);

    // a null sink is a safe no-op
    CHECK(bridge.register_all_to_script(&sink, nullptr) == 0);
    CHECK(sink.names.size() == 2);
}

TEST_CASE("okn::ecs::ScriptingBridge - add/read/write/query components by name on the live World") {
    World w;
    ScriptingBridge bridge(w);
    bridge.register_component<Position>("Position");
    bridge.register_component<Velocity>("Velocity");

    // Script-style flow: create an entity, attach + WRITE a component purely by name.
    Entity a = ScriptingBridge::script_create_entity(w);
    void* pa = bridge.add_component(a, "Position");
    REQUIRE(pa != nullptr);
    static_cast<Position*>(pa)->x = 5.0f;
    static_cast<Position*>(pa)->y = 6.0f;

    // READ it back by name — and confirm it is the SAME storage the typed API sees.
    const void* ra = bridge.component_data(a, "Position");
    REQUIRE(ra != nullptr);
    CHECK(static_cast<const Position*>(ra)->x == 5.0f);
    CHECK(static_cast<const Position*>(ra)->y == 6.0f);
    const Position* typed = w.get_component<Position>(a);
    REQUIRE(typed != nullptr);
    CHECK(typed->x == 5.0f);
    CHECK(typed->y == 6.0f);

    // A second entity carrying both components.
    Entity b = ScriptingBridge::script_create_entity(w);
    bridge.add_component(b, "Position");
    bridge.add_component(b, "Velocity");

    // QUERY by name returns exactly the entities with that component.
    CHECK(bridge.query("Position").size() == 2);
    const auto with_vel = bridge.query("Velocity");
    REQUIRE(with_vel.size() == 1);
    CHECK(with_vel[0] == b);

    // Unknown / null names are inert (no throw, no insertion).
    CHECK(bridge.add_component(a, "Nope") == nullptr);
    CHECK(bridge.component_data(a, nullptr) == nullptr);
    CHECK(bridge.query("Nope").empty());

    // Destroying via the script API drops the entity's components from queries.
    ScriptingBridge::script_destroy_entity(w, a);
    CHECK(bridge.component_data(a, "Position") == nullptr);
    CHECK(bridge.query("Position").size() == 1);
}
