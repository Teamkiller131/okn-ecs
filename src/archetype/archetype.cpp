#include <okn/ecs/archetype/archetype.hpp>

namespace okn::ecs {

Archetype::Archetype(std::vector<ComponentTypeId> components)
    : components_(std::move(components)) {
    std::sort(components_.begin(), components_.end());
}

auto Archetype::has_component(ComponentTypeId id) const -> bool {
    return std::binary_search(components_.begin(), components_.end(), id);
}

auto Archetype::component_index(ComponentTypeId id) const -> int {
    auto it = std::lower_bound(components_.begin(), components_.end(), id);
    if (it != components_.end() && *it == id) {
        return static_cast<int>(std::distance(components_.begin(), it));
    }
    return -1;
}

auto Archetype::add_edge(ComponentTypeId id) -> Edge& {
    for (auto& e : edges_) {
        if (e.component == id) {
            return e;
        }
    }
    edges_.push_back(Edge{id, nullptr});
    return edges_.back();
}

auto Archetype::get_edge(ComponentTypeId id) const -> const Edge* {
    for (const auto& e : edges_) {
        if (e.component == id) {
            return &e;
        }
    }
    return nullptr;
}

} // namespace okn::ecs
