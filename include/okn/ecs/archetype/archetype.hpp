#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <vector>
#include <algorithm>

namespace okn::ecs {

class Chunk;

class Archetype {
public:
    struct Edge {
        ComponentTypeId component = 0;
        Archetype* archetype = nullptr;
    };

    Archetype() = default;

    explicit Archetype(std::vector<ComponentTypeId> components);

    auto type_mask() const -> const std::vector<ComponentTypeId>& { return components_; }
    auto has_component(ComponentTypeId id) const -> bool;
    auto component_index(ComponentTypeId id) const -> int;
    auto component_count() const -> usize { return components_.size(); }

    auto add_edge(ComponentTypeId id) -> Edge&;
    auto get_edge(ComponentTypeId id) const -> const Edge*;

    auto edges() const -> const std::vector<Edge>& { return edges_; }

    Chunk* chunks_ = nullptr;
    usize chunk_count_ = 0;

private:
    std::vector<ComponentTypeId> components_;
    std::vector<Edge> edges_;
};

} // namespace okn::ecs
