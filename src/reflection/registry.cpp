#include <okn/ecs/reflection/registry.hpp>

namespace okn::ecs {

auto ComponentRegistry::instance() -> ComponentRegistry& {
    static ComponentRegistry reg;
    return reg;
}

auto ComponentRegistry::get_info(ComponentTypeId id) const -> const ComponentInfo* {
    auto it = components_.find(id);
    return (it != components_.end()) ? &it->second : nullptr;
}

auto ComponentRegistry::get_type_name(ComponentTypeId id) const -> std::string {
    auto it = id_to_type_.find(id);
    return (it != id_to_type_.end()) ? it->second : "unknown";
}

auto ComponentRegistry::all_components() const -> const std::unordered_map<ComponentTypeId, ComponentInfo>& {
    return components_;
}

} // namespace okn::ecs