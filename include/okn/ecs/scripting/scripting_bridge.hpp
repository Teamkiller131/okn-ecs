#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/world.hpp>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace okn::ecs {

// Runtime ECS surface exposed to a script runtime.
//
// Components are registered under a human name keyed to the SAME ComponentTypeId
// the World uses for its stores (World::component_type_id<T>()), so a name-based
// query coming from a script resolves to real component storage.
//
// NOTE: this deliberately does NOT use the global ComponentRegistry — that one
// hashes __FUNCSIG__, which does not match the World's typeid-name store keys,
// so it cannot resolve a name to a live store. The bridge keeps its own
// consistent map instead.
class ScriptingBridge {
public:
    struct ComponentDesc {
        ComponentTypeId id = 0;  // == World::component_type_id<T>()
        usize size = 0;
    };

    // Sink the host uses to forward each registered component to its own script
    // context, so the bridge never has to depend on okn-script.
    using RegisterFn = void (*)(void* script_ctx, const char* name,
                                ComponentTypeId id, usize size);

    explicit ScriptingBridge(World& world);

    // Make component T scriptable under `name`, keyed by the World's store id.
    template <class T>
    void register_component(std::string name) {
        descs_.insert_or_assign(std::move(name),
                                ComponentDesc{World::component_type_id<T>(), sizeof(T)});
    }

    // Forward every registered component to a script context via `fn`. A null
    // `fn` is a no-op. Returns the number of components forwarded.
    auto register_all_to_script(void* script_ctx, RegisterFn fn) const -> usize;

    // Runtime, name-based component query for scripts. False if `name` was never
    // registered or the entity lacks that component.
    [[nodiscard]] auto has_component(Entity e, const char* name) const -> bool;

    // Read/write a registered component's raw bytes by name (null if `name` is unknown
    // or the entity lacks it). The script knows the layout from registration and casts.
    [[nodiscard]] auto component_data(Entity e, const char* name) -> void*;
    [[nodiscard]] auto component_data(Entity e, const char* name) const -> const void*;

    // Attach a zero-initialized instance of the named component to `e`; returns its
    // data (null if `name` was never registered). Existing data is left untouched.
    auto add_component(Entity e, const char* name) -> void*;

    // Every live entity that currently has the named component (empty if unknown).
    [[nodiscard]] auto query(const char* name) const -> std::vector<Entity>;

    // Resolve a registered name to its ComponentTypeId (0 if unknown).
    [[nodiscard]] auto component_id(const char* name) const -> ComponentTypeId;

    [[nodiscard]] auto registered_count() const -> usize { return descs_.size(); }

    // The registered name -> descriptor map, for generic consumers that enumerate the
    // scriptable surface (the editor's generic inspector, tooling): iterate, then use
    // has_component/component_data per name.
    [[nodiscard]] auto descriptors() const
        -> const std::unordered_map<std::string, ComponentDesc>& { return descs_; }

    // Entity lifetime API for scripts.
    static auto script_create_entity(World& w) -> Entity;
    static void script_destroy_entity(World& w, Entity e);

private:
    World* world_;
    std::unordered_map<std::string, ComponentDesc> descs_;
};

} // namespace okn::ecs
