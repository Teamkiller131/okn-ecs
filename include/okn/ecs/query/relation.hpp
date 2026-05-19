#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/entity.hpp>
#include <unordered_map>
#include <vector>

namespace okn::ecs {

class Relation {
public:
    Relation() = default;

    void set_parent(World& /*world*/, Entity child, Entity parent) {
        u32 child_idx = child.index();
        u32 parent_idx = parent.index();

        if (child_idx < child_to_parent_.size() && child_to_parent_[child_idx] != ~0u) {
            u32 old_parent = child_to_parent_[child_idx];
            auto& siblings = parent_to_children_[old_parent];
            siblings.erase(std::remove(siblings.begin(), siblings.end(), child_idx), siblings.end());
        }

        if (child_idx >= child_to_parent_.size()) {
            child_to_parent_.resize(child_idx + 1, ~0u);
        }
        child_to_parent_[child_idx] = parent_idx;
        parent_to_children_[parent_idx].push_back(child_idx);
    }

    auto get_parent(Entity child) const -> Entity {
        u32 child_idx = child.index();
        if (child_idx >= child_to_parent_.size()) return kInvalidEntity;
        u32 parent_idx = child_to_parent_[child_idx];
        if (parent_idx == ~0u) return kInvalidEntity;
        return make_entity(parent_idx, 0);
    }

    auto get_children(Entity parent) const -> std::vector<Entity> {
        u32 parent_idx = parent.index();
        std::vector<Entity> result;
        auto it = parent_to_children_.find(parent_idx);
        if (it != parent_to_children_.end()) {
            for (u32 child_idx : it->second) {
                result.push_back(make_entity(child_idx, 0));
            }
        }
        return result;
    }

    void remove_parent(Entity child) {
        u32 child_idx = child.index();
        if (child_idx >= child_to_parent_.size()) return;
        u32 parent_idx = child_to_parent_[child_idx];
        if (parent_idx == ~0u) return;

        auto& siblings = parent_to_children_[parent_idx];
        siblings.erase(std::remove(siblings.begin(), siblings.end(), child_idx), siblings.end());
        child_to_parent_[child_idx] = ~0u;
    }

private:
    std::unordered_map<u32, std::vector<u32>> parent_to_children_;
    std::vector<u32> child_to_parent_;
};

} // namespace okn::ecs
