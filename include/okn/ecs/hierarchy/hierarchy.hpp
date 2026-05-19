#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/entity.hpp>
#include <unordered_map>
#include <vector>

namespace okn::ecs {

class World;

class Hierarchy {
public:
    void set_parent(Entity child, Entity parent);
    auto get_parent(Entity entity) const -> Entity;
    auto get_children(Entity entity) const -> const std::vector<Entity>&;
    void remove_child(Entity entity);

    void update_transforms(World& world);

    auto dirty_count() const -> usize { return dirty_.size(); }

private:
    std::unordered_map<u32, Entity> child_to_parent_;
    std::unordered_map<u32, std::vector<Entity>> parent_to_children_;
    std::vector<Entity> dirty_;

    bool is_dirty(Entity e) const;
    void mark_dirty(Entity e);
};

} // namespace okn::ecs
