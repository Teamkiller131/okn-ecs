#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/reflection/registry.hpp>

namespace okn::ecs {

class World;

class ScriptingBridge {
public:
    ScriptingBridge(World& world);

    // Register all components with the script runtime
    void register_all_to_script(void* script_ctx); // void* = IScriptContext*

    // Entity API for scripts
    static auto script_create_entity(World& w) -> Entity;
    static void script_destroy_entity(World& w, Entity e);
    static auto script_has_component(World& w, Entity e, const char* type_name) -> bool;

private:
    World* world_;
};

} // namespace okn::ecs