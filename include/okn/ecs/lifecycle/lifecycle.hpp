#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/entity.hpp>
#include <okn/ecs/component.hpp>
#include <functional>
#include <vector>
#include <unordered_map>

namespace okn::ecs {

class World;

class LifecycleHooks {
public:
    using HookFn = std::function<void(World&, Entity)>;

    template <class T>
    void on_add(HookFn fn) {
        add_hooks_[component_type_id<T>()].push_back(std::move(fn));
    }

    template <class T>
    void on_remove(HookFn fn) {
        remove_hooks_[component_type_id<T>()].push_back(std::move(fn));
    }

    template <class T>
    void on_move(HookFn fn) {
        move_hooks_[component_type_id<T>()].push_back(std::move(fn));
    }

    void notify_add(ComponentTypeId type, World& w, Entity e);
    void notify_remove(ComponentTypeId type, World& w, Entity e);
    void notify_move(ComponentTypeId type, World& w, Entity e);

    template <class T>
    static auto component_type_id() -> ComponentTypeId {
        return ComponentInfo::from_type<T>().type_id;
    }

private:
    std::unordered_map<ComponentTypeId, std::vector<HookFn>> add_hooks_;
    std::unordered_map<ComponentTypeId, std::vector<HookFn>> remove_hooks_;
    std::unordered_map<ComponentTypeId, std::vector<HookFn>> move_hooks_;
};

} // namespace okn::ecs
