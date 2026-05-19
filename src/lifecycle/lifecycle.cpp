#include <okn/ecs/lifecycle/lifecycle.hpp>

namespace okn::ecs {

void LifecycleHooks::notify_add(ComponentTypeId type, World& w, Entity e) {
    auto it = add_hooks_.find(type);
    if (it != add_hooks_.end()) {
        for (auto& fn : it->second) {
            fn(w, e);
        }
    }
}

void LifecycleHooks::notify_remove(ComponentTypeId type, World& w, Entity e) {
    auto it = remove_hooks_.find(type);
    if (it != remove_hooks_.end()) {
        for (auto& fn : it->second) {
            fn(w, e);
        }
    }
}

void LifecycleHooks::notify_move(ComponentTypeId type, World& w, Entity e) {
    auto it = move_hooks_.find(type);
    if (it != move_hooks_.end()) {
        for (auto& fn : it->second) {
            fn(w, e);
        }
    }
}

} // namespace okn::ecs
