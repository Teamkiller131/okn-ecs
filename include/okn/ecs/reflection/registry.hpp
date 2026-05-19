#pragma once

#include <okn/ecs/component.hpp>
#include <unordered_map>
#include <string>

namespace okn::ecs {

class ComponentRegistry {
public:
    static auto instance() -> ComponentRegistry&;

    template <class T>
    void register_component(const std::string& name) {
        ComponentInfo info = ComponentInfo::from_type<T>();
        info.name = name;
        components_[info.type_id] = info;
        id_to_type_[info.type_id] = name;
    }

    auto get_info(ComponentTypeId id) const -> const ComponentInfo*;
    auto get_type_name(ComponentTypeId id) const -> std::string;
    auto all_components() const -> const std::unordered_map<ComponentTypeId, ComponentInfo>&;

    template <class T>
    static auto type_id() -> ComponentTypeId { return ComponentInfo::from_type<T>().type_id; }

private:
    ComponentRegistry() = default;
    std::unordered_map<ComponentTypeId, ComponentInfo> components_;
    std::unordered_map<ComponentTypeId, std::string> id_to_type_;
};

} // namespace okn::ecs