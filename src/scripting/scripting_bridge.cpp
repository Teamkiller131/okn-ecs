#include <okn/ecs/scripting/scripting_bridge.hpp>
#include <okn/ecs/world.hpp>

namespace okn::ecs {

ScriptingBridge::ScriptingBridge(World& world) : world_(&world) {}

auto ScriptingBridge::register_all_to_script(void* script_ctx, RegisterFn fn) const -> usize {
    if (fn == nullptr) { return 0; }
    usize n = 0;
    for (const auto& [name, desc] : descs_) {
        fn(script_ctx, name.c_str(), desc.id, desc.size);
        ++n;
    }
    return n;
}

auto ScriptingBridge::component_id(const char* name) const -> ComponentTypeId {
    if (name == nullptr) { return 0; }
    auto it = descs_.find(name);
    return it == descs_.end() ? ComponentTypeId{0} : it->second.id;
}

auto ScriptingBridge::has_component(Entity e, const char* name) const -> bool {
    if (name == nullptr) { return false; }
    auto it = descs_.find(name);
    if (it == descs_.end()) { return false; }
    return world_->has_component_by_id(e, it->second.id);
}

auto ScriptingBridge::component_data(Entity e, const char* name) -> void* {
    const ComponentTypeId id = component_id(name);
    return (id == 0) ? nullptr : world_->component_data_by_id(e, id);
}

auto ScriptingBridge::component_data(Entity e, const char* name) const -> const void* {
    const ComponentTypeId id = component_id(name);
    return (id == 0) ? nullptr : world_->component_data_by_id(e, id);
}

auto ScriptingBridge::add_component(Entity e, const char* name) -> void* {
    if (name == nullptr) { return nullptr; }
    auto it = descs_.find(name);
    if (it == descs_.end()) { return nullptr; }
    return world_->add_component_by_id(e, it->second.id, it->second.size);
}

auto ScriptingBridge::query(const char* name) const -> std::vector<Entity> {
    const ComponentTypeId id = component_id(name);
    return (id == 0) ? std::vector<Entity>{} : world_->entities_with(id);
}

auto ScriptingBridge::fields(const char* comp) const -> const std::vector<FieldDesc>* {
    if (comp == nullptr) { return nullptr; }
    const auto it = descs_.find(comp);
    return it == descs_.end() ? nullptr : &it->second.fields;
}

auto ScriptingBridge::find_field(const char* comp, const char* field) const -> const FieldDesc* {
    if (field == nullptr) { return nullptr; }
    const auto* fs = fields(comp);
    if (fs == nullptr) { return nullptr; }
    for (const FieldDesc& f : *fs) {
        if (f.name == field) { return &f; }
    }
    return nullptr;
}

auto ScriptingBridge::field_data(Entity e, const char* comp, const char* field) -> void* {
    const FieldDesc* fd = find_field(comp, field);
    if (fd == nullptr) { return nullptr; }
    void* base = component_data(e, comp);
    return base == nullptr ? nullptr : static_cast<u8*>(base) + fd->offset;
}

auto ScriptingBridge::script_create_entity(World& w) -> Entity { return w.create_entity(); }

void ScriptingBridge::script_destroy_entity(World& w, Entity e) {
    if (w.entity_alive(e)) { w.destroy_entity(e); }
}

} // namespace okn::ecs
