#include <okn/ecs/scripting/scripting_bridge.hpp>
#include <okn/ecs/world.hpp>

namespace okn::ecs {

ScriptingBridge::ScriptingBridge(World& world) : world_(&world) {}

void ScriptingBridge::register_all_to_script(void* script_ctx) {
    (void)script_ctx;
    auto& reg = ComponentRegistry::instance();
    auto& comps = reg.all_components();
    for (auto& [id, info] : comps) {
        (void)id; (void)info;
        // TODO: call script_ctx->register_component(info.name, info.size) when lua binding is complete
    }
}

auto ScriptingBridge::script_create_entity(World& w) -> Entity { return w.create_entity(); }

void ScriptingBridge::script_destroy_entity(World& w, Entity e) { if(w.entity_alive(e)) w.destroy_entity(e); }

auto ScriptingBridge::script_has_component(World& w, Entity e, const char* type_name) -> bool {
    auto type_id = okn::core::hash_string_view(type_name);
    // World template methods require compile-time type; for runtime queries use reflection
    (void)w; (void)e; (void)type_id;
    return false;
}

} // namespace okn::ecs