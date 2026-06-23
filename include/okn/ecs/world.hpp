#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/entity.hpp>
#include <okn/ecs/component.hpp>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <typeinfo>

namespace okn::ecs {

template <class... Components>
class View;

class Query;
class Serializer;
class Deserializer;

class World {
    friend class Query;
    friend class WorldBuilder;
    friend class Serializer;     // walks stores_ to snapshot component bytes
    friend class Deserializer;   // recreates entities + re-attaches component bytes

public:
    World();
    ~World();

    World(const World&) = delete;
    auto operator=(const World&) -> World& = delete;
    World(World&&) noexcept;
    auto operator=(World&&) noexcept -> World&;

    auto create_entity() -> Entity;
    void destroy_entity(Entity entity);
    auto entity_alive(Entity entity) const -> bool;
    auto entity_count() const -> usize;

    template <class T>
    void add_component(Entity entity, const T& value);

    template <class T>
    void remove_component(Entity entity);

    template <class T>
    auto get_component(Entity entity) -> T*;

    template <class T>
    auto get_component(Entity entity) const -> const T*;

    template <class T>
    auto has_component(Entity entity) const -> bool;

    // Runtime, type-erased existence check: does `entity` have the component
    // whose store key is `id` (== component_type_id<T>())? Lets reflection /
    // scripting query components by a runtime id without the compile-time type.
    [[nodiscard]] auto has_component_by_id(Entity entity, ComponentTypeId id) const -> bool;

    template <class... Components>
    auto query() -> View<Components...>;

    template <class T>
    static auto component_type_id() -> ComponentTypeId;

private:
    struct ComponentStore {
        std::vector<u8> data;
        std::vector<Entity> entities;
        std::vector<u32> sparse;
        usize component_size = 0;
        usize count = 0;

        void push(Entity entity, const void* value);
        void remove(Entity entity);
        auto get(Entity entity) const -> const void*;
        auto get(Entity entity) -> void*;
        auto has(Entity entity) const -> bool;
    };

    std::unordered_map<ComponentTypeId, ComponentStore> stores_;
    std::vector<u32> entity_generations_;
    std::vector<u32> free_entity_indices_;
    u32 next_entity_index_ = 0;
    usize alive_count_ = 0;

    void ensure_sparse_capacity(ComponentStore& store, u32 entity_index);
    void remove_all_components(Entity entity);

    template <class T>
    auto ensure_store() -> ComponentStore&;
};

} // namespace okn::ecs

#include <okn/ecs/query/view.hpp>

// ── Template implementations ──

namespace okn::ecs {

template <class T>
inline auto World::component_type_id() -> ComponentTypeId {
    static const ComponentTypeId id = []() {
        return okn::core::hash_string_view(typeid(T).name());
    }();
    return id;
}

template <class T>
inline auto World::ensure_store() -> ComponentStore& {
    auto cid = component_type_id<T>();
    auto it = stores_.find(cid);
    if (it == stores_.end()) {
        auto [inserted, _] = stores_.emplace(cid, ComponentStore{});
        inserted->second.component_size = sizeof(T);
        return inserted->second;
    }
    return it->second;
}

template <class T>
inline void World::add_component(Entity entity, const T& value) {
    u32 eidx = entity.index();
    auto& store = ensure_store<T>();
    ensure_sparse_capacity(store, eidx);
    if (store.has(entity)) {
        std::memcpy(store.get(entity), &value, sizeof(T));
    } else {
        store.push(entity, &value);
    }
}

template <class T>
inline void World::remove_component(Entity entity) {
    auto cid = component_type_id<T>();
    auto it = stores_.find(cid);
    if (it != stores_.end()) {
        it->second.remove(entity);
    }
}

template <class T>
inline auto World::get_component(Entity entity) -> T* {
    auto cid = component_type_id<T>();
    auto it = stores_.find(cid);
    if (it != stores_.end()) {
        return static_cast<T*>(it->second.get(entity));
    }
    return nullptr;
}

template <class T>
inline auto World::get_component(Entity entity) const -> const T* {
    auto cid = component_type_id<T>();
    auto it = stores_.find(cid);
    if (it != stores_.end()) {
        return static_cast<const T*>(it->second.get(entity));
    }
    return nullptr;
}

template <class T>
inline auto World::has_component(Entity entity) const -> bool {
    auto cid = component_type_id<T>();
    auto it = stores_.find(cid);
    if (it != stores_.end()) {
        return it->second.has(entity);
    }
    return false;
}

inline auto World::has_component_by_id(Entity entity, ComponentTypeId id) const -> bool {
    auto it = stores_.find(id);
    return it != stores_.end() && it->second.has(entity);
}

template <class... Components>
inline auto World::query() -> View<Components...> {
    std::vector<Entity> matched;

    if constexpr (sizeof...(Components) > 0) {
        ComponentTypeId cids[] = {component_type_id<Components>()...};
        constexpr usize n = sizeof...(Components);

        const ComponentStore* smallest = nullptr;
        ComponentTypeId smallest_cid = 0;

        for (usize i = 0; i < n; ++i) {
            auto it = stores_.find(cids[i]);
            if (it == stores_.end()) {
                return View<Components...>(*this, std::move(matched));
            }
            if (!smallest || it->second.count < smallest->count) {
                smallest = &it->second;
                smallest_cid = cids[i];
            }
        }

        for (usize i = 0; i < smallest->count; ++i) {
            Entity entity = smallest->entities[i];
            bool has_all = true;
            for (usize j = 0; j < n && has_all; ++j) {
                if (cids[j] == smallest_cid) continue;
                auto it = stores_.find(cids[j]);
                if (it == stores_.end() || !it->second.has(entity)) {
                    has_all = false;
                }
            }
            if (has_all) {
                matched.push_back(entity);
            }
        }
    }

    return View<Components...>(*this, std::move(matched));
}

} // namespace okn::ecs
