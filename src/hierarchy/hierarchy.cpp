#include <okn/ecs/hierarchy/hierarchy.hpp>
#include <okn/ecs/hierarchy/transform.hpp>
#include <okn/ecs/world.hpp>
#include <algorithm>
#include <set>

namespace okn::ecs {

void Hierarchy::set_parent(Entity child, Entity parent) {
    if (!child.is_valid() || !parent.is_valid()) return;
    if (child == parent) return;

    u32 child_idx = child.index();

    auto existing = child_to_parent_.find(child_idx);
    if (existing != child_to_parent_.end()) {
        if (existing->second == parent) return;
        u32 old_parent_idx = existing->second.index();
        auto& siblings = parent_to_children_[old_parent_idx];
        siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
    }

    child_to_parent_[child_idx] = parent;
    parent_to_children_[parent.index()].push_back(child);

    mark_dirty(child);
}

auto Hierarchy::get_parent(Entity entity) const -> Entity {
    u32 idx = entity.index();
    auto it = child_to_parent_.find(idx);
    if (it != child_to_parent_.end()) {
        return it->second;
    }
    return kInvalidEntity;
}

auto Hierarchy::get_children(Entity entity) const -> const std::vector<Entity>& {
    u32 idx = entity.index();
    auto it = parent_to_children_.find(idx);
    if (it != parent_to_children_.end()) {
        return it->second;
    }
    static const std::vector<Entity> empty;
    return empty;
}

void Hierarchy::remove_child(Entity entity) {
    u32 idx = entity.index();

    auto parent_it = child_to_parent_.find(idx);
    if (parent_it != child_to_parent_.end()) {
        Entity parent = parent_it->second;
        auto& siblings = parent_to_children_[parent.index()];
        siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());
        child_to_parent_.erase(parent_it);
    }

    auto children_it = parent_to_children_.find(idx);
    if (children_it != parent_to_children_.end()) {
        for (auto child : children_it->second) {
            u32 cidx = child.index();
            child_to_parent_.erase(cidx);
            mark_dirty(child);
        }
        parent_to_children_.erase(children_it);
    }

    mark_dirty(entity);
}

bool Hierarchy::is_dirty(Entity e) const {
    return std::find(dirty_.begin(), dirty_.end(), e) != dirty_.end();
}

void Hierarchy::mark_dirty(Entity e) {
    if (!is_dirty(e)) {
        dirty_.push_back(e);
    }
    auto children_it = parent_to_children_.find(e.index());
    if (children_it != parent_to_children_.end()) {
        for (auto child : children_it->second) {
            mark_dirty(child);
        }
    }
}

void Hierarchy::update_transforms(World& world) {
    if (dirty_.empty()) return;

    std::set<u64> needs_update;
    for (auto e : dirty_) {
        Entity cur = e;
        while (cur.is_valid()) {
            needs_update.insert(cur.raw());
            Entity p = get_parent(cur);
            if (!p.is_valid()) break;
            cur = p;
        }
    }

    std::vector<Entity> sorted;
    for (u64 raw : needs_update) {
        sorted.push_back(Entity(static_cast<u32>(raw & 0xFFFFFFFFULL),
                                static_cast<u32>(raw >> 32)));
    }

    auto depth = [this](Entity e) -> int {
        int d = 0;
        Entity cur = e;
        while (true) {
            Entity p = get_parent(cur);
            if (!p.is_valid()) break;
            cur = p;
            ++d;
        }
        return d;
    };

    std::sort(sorted.begin(), sorted.end(), [&](Entity a, Entity b) {
        return depth(a) < depth(b);
    });

    for (auto e : sorted) {
        Entity parent = get_parent(e);
        const float* parent_mat = nullptr;

        if (parent.is_valid()) {
            auto* parent_wt = world.get_component<WorldTransform>(parent);
            if (parent_wt) {
                parent_mat = parent_wt->mat;
            }
        }

        auto* local = world.get_component<LocalTransform>(e);
        if (local) {
            WorldTransform wt = compute_world_matrix(*local, parent_mat);
            world.add_component(e, wt);
        }
    }

    dirty_.clear();
}

} // namespace okn::ecs
