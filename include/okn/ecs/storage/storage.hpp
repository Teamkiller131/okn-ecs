#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/component.hpp>
#include <okn/ecs/entity.hpp>
#include <okn/ecs/archetype/archetype.hpp>
#include <okn/ecs/archetype/chunk.hpp>
#include <okn/ecs/archetype/chunk_allocator.hpp>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <algorithm>

namespace okn::ecs {

class Storage {
public:
    Storage();
    ~Storage();

    Storage(const Storage&) = delete;
    auto operator=(const Storage&) -> Storage& = delete;

    template <class T>
    void register_component() {
        ComponentInfo info = ComponentInfo::from_type<T>();
        register_component_info(info);
    }

    template <class T>
    void add_component(Entity entity, const T& value) {
        auto type_id = ComponentInfo::from_type<T>().type_id;
        if (!register_component_info(ComponentInfo::from_type<T>())) {
            add_component_internal(entity, type_id, &value, sizeof(T));
            return;
        }
        add_component_internal(entity, type_id, &value, sizeof(T));
    }

    template <class T>
    void remove_component(Entity entity) {
        auto type_id = ComponentInfo::from_type<T>().type_id;
        remove_component_internal(entity, type_id);
    }

    template <class T>
    auto get_component(Entity entity) -> T* {
        auto type_id = ComponentInfo::from_type<T>().type_id;
        return static_cast<T*>(get_component_internal(entity, type_id));
    }

    template <class T>
    auto get_component(Entity entity) const -> const T* {
        auto type_id = ComponentInfo::from_type<T>().type_id;
        return static_cast<const T*>(get_component_internal(entity, type_id));
    }

    template <class T>
    auto has_component(Entity entity) -> bool {
        auto type_id = ComponentInfo::from_type<T>().type_id;
        return has_component_internal(entity, type_id);
    }

    auto find_archetype(const std::vector<ComponentTypeId>& types) -> Archetype*;
    auto get_or_create_archetype(const std::vector<ComponentTypeId>& types) -> Archetype*;

    auto create_entity() -> Entity;
    void destroy_entity(Entity entity);
    auto entity_alive(Entity entity) const -> bool;

    auto archetypes() const -> const std::vector<Archetype*>& { return archetypes_; }
    auto get_component_info(ComponentTypeId id) const -> const ComponentInfo*;

private:
    std::vector<Archetype*> archetypes_;
    std::vector<Entity> entity_pool_;
    std::vector<u32> entity_generations_;
    u32 next_entity_index_ = 0;
    ChunkAllocator chunk_allocator_;
    std::unordered_map<u64, Archetype*> archetype_map_;

    std::vector<Archetype*> entity_archetypes_;
    std::vector<Chunk*> entity_chunks_;
    std::vector<usize> entity_rows_;

    std::unordered_map<ComponentTypeId, ComponentInfo> component_registry_;

    auto ensure_entity_storage(usize index) -> void;
    auto hash_component_set(const std::vector<ComponentTypeId>& types) const -> u64;

    auto register_component_info(const ComponentInfo& info) -> bool;
    auto add_component_internal(Entity entity, ComponentTypeId type_id, const void* value, usize size) -> void;
    auto remove_component_internal(Entity entity, ComponentTypeId type_id) -> void;
    auto get_component_internal(Entity entity, ComponentTypeId type_id) -> void*;
    auto get_component_internal(Entity entity, ComponentTypeId type_id) const -> const void*;
    auto has_component_internal(Entity entity, ComponentTypeId type_id) const -> bool;

    auto move_entity(Entity entity, Archetype* from_arch, Archetype* to_arch, ComponentTypeId new_type, const void* new_value, usize new_size) -> void;
    auto move_entity_remove(Entity entity, Archetype* from_arch, Archetype* to_arch, ComponentTypeId removed_type) -> void;
};

} // namespace okn::ecs
