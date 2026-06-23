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
