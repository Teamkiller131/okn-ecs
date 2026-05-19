#include <okn/ecs/storage/storage.hpp>
#include <okn/core/api/hash.hpp>

namespace okn::ecs {

Storage::Storage() {
    get_or_create_archetype({});
}

Storage::~Storage() {
    for (auto* arch : archetypes_) {
        Chunk* chunk = arch->chunks_;
        while (chunk) {
            Chunk* next = chunk->next;
            delete chunk;
            chunk = next;
        }
        delete arch;
    }
}

auto Storage::ensure_entity_storage(usize index) -> void {
    if (index >= entity_generations_.size()) {
        entity_generations_.resize(index + 1, 0);
        entity_archetypes_.resize(index + 1, nullptr);
        entity_chunks_.resize(index + 1, nullptr);
        entity_rows_.resize(index + 1, static_cast<usize>(-1));
    }
}

auto Storage::hash_component_set(const std::vector<ComponentTypeId>& types) const -> u64 {
    u64 h = 14695981039346656037ULL;
    for (auto id : types) {
        h ^= id;
        h *= 1099511628211ULL;
    }
    return h;
}

auto Storage::register_component_info(const ComponentInfo& info) -> bool {
    auto [it, inserted] = component_registry_.try_emplace(info.type_id, info);
    return inserted;
}

auto Storage::find_archetype(const std::vector<ComponentTypeId>& types) -> Archetype* {
    auto sorted = types;
    std::sort(sorted.begin(), sorted.end());
    u64 h = hash_component_set(sorted);
    auto it = archetype_map_.find(h);
    if (it != archetype_map_.end()) {
        return it->second;
    }
    return nullptr;
}

auto Storage::get_or_create_archetype(const std::vector<ComponentTypeId>& types) -> Archetype* {
    auto sorted = types;
    std::sort(sorted.begin(), sorted.end());
    u64 h = hash_component_set(sorted);
    auto it = archetype_map_.find(h);
    if (it != archetype_map_.end()) {
        return it->second;
    }
    auto* arch = new Archetype(sorted);
    archetypes_.push_back(arch);
    archetype_map_[h] = arch;
    return arch;
}

auto Storage::create_entity() -> Entity {
    if (!entity_pool_.empty()) {
        Entity e = entity_pool_.back();
        entity_pool_.pop_back();
        u32 index = e.index();
        u32 gen = e.generation() + 1;
        Entity recycled(index, gen);
        entity_generations_[index] = gen;
        entity_archetypes_[index] = nullptr;
        entity_chunks_[index] = nullptr;
        entity_rows_[index] = static_cast<usize>(-1);
        return recycled;
    }
    u32 index = next_entity_index_++;
    ensure_entity_storage(index);
    entity_generations_[index] = 0;
    Entity e(index, 0);
    return e;
}

void Storage::destroy_entity(Entity entity) {
    u32 index = entity.index();
    if (index >= entity_generations_.size()) return;
    if (entity_generations_[index] != entity.generation()) return;

    Archetype* arch = entity_archetypes_[index];
    if (arch) {
        Chunk* chunk = entity_chunks_[index];
        usize row = entity_rows_[index];
        if (chunk && row != static_cast<usize>(-1)) {
            chunk->remove_entity(row);
        }
    }

    entity_archetypes_[index] = nullptr;
    entity_chunks_[index] = nullptr;
    entity_rows_[index] = static_cast<usize>(-1);
    entity_generations_[index]++;

    entity_pool_.push_back(Entity(index, entity_generations_[index]));
}

auto Storage::entity_alive(Entity entity) const -> bool {
    u32 index = entity.index();
    if (index >= entity_generations_.size()) return false;
    return entity_generations_[index] == entity.generation();
}

auto Storage::get_component_info(ComponentTypeId id) const -> const ComponentInfo* {
    auto it = component_registry_.find(id);
    if (it != component_registry_.end()) {
        return &it->second;
    }
    return nullptr;
}

auto Storage::add_component_internal(Entity entity, ComponentTypeId type_id, const void* value, usize size) -> void {
    u32 index = entity.index();
    ensure_entity_storage(index);

    Archetype* current_arch = entity_archetypes_[index];

    if (current_arch) {
        int comp_idx = current_arch->component_index(type_id);
        if (comp_idx >= 0) {
            Chunk* chunk = entity_chunks_[index];
            usize row = entity_rows_[index];
            void* dst = chunk->get_component_ptr(row, static_cast<usize>(comp_idx), size);
            if (dst) {
                std::memcpy(dst, value, size);
            }
            return;
        }

        std::vector<ComponentTypeId> new_types = current_arch->type_mask();
        new_types.push_back(type_id);
        std::sort(new_types.begin(), new_types.end());
        Archetype* target_arch = get_or_create_archetype(new_types);

        move_entity(entity, current_arch, target_arch, type_id, value, size);
    } else {
        std::vector<ComponentTypeId> new_types = {type_id};
        Archetype* target_arch = get_or_create_archetype(new_types);

        std::vector<usize> comp_sizes;
        for (auto cid : target_arch->type_mask()) {
            const ComponentInfo* info = get_component_info(cid);
            comp_sizes.push_back(info ? info->size : 8);
        }

        Chunk* chunk = target_arch->chunks_;
        while (chunk && chunk->is_full()) {
            chunk = chunk->next;
        }
        if (!chunk) {
            chunk = chunk_allocator_.allocate(*target_arch, comp_sizes);
            chunk->next = target_arch->chunks_;
            target_arch->chunks_ = chunk;
            target_arch->chunk_count_++;
        }

        usize row = chunk->add_entity(entity);

        int new_comp_idx = target_arch->component_index(type_id);
        if (new_comp_idx >= 0) {
            void* dst = chunk->get_component_ptr(row, static_cast<usize>(new_comp_idx), size);
            if (dst) {
                std::memcpy(dst, value, size);
            }
        }

        entity_archetypes_[index] = target_arch;
        entity_chunks_[index] = chunk;
        entity_rows_[index] = row;
    }
}

auto Storage::remove_component_internal(Entity entity, ComponentTypeId type_id) -> void {
    u32 index = entity.index();
    if (index >= entity_generations_.size()) return;

    Archetype* current_arch = entity_archetypes_[index];
    if (!current_arch) return;

    if (!current_arch->has_component(type_id)) return;

    std::vector<ComponentTypeId> new_types;
    for (auto cid : current_arch->type_mask()) {
        if (cid != type_id) {
            new_types.push_back(cid);
        }
    }
    std::sort(new_types.begin(), new_types.end());
    Archetype* target_arch = get_or_create_archetype(new_types);

    move_entity_remove(entity, current_arch, target_arch, type_id);
}

auto Storage::get_component_internal(Entity entity, ComponentTypeId type_id) -> void* {
    u32 index = entity.index();
    if (index >= entity_generations_.size()) return nullptr;

    Archetype* arch = entity_archetypes_[index];
    if (!arch) return nullptr;

    int comp_idx = arch->component_index(type_id);
    if (comp_idx < 0) return nullptr;

    Chunk* chunk = entity_chunks_[index];
    usize row = entity_rows_[index];
    if (!chunk || row == static_cast<usize>(-1)) return nullptr;

    const ComponentInfo* info = get_component_info(type_id);
    usize comp_size = info ? info->size : 8;
    return chunk->get_component_ptr(row, static_cast<usize>(comp_idx), comp_size);
}

auto Storage::get_component_internal(Entity entity, ComponentTypeId type_id) const -> const void* {
    u32 index = entity.index();
    if (index >= entity_generations_.size()) return nullptr;

    Archetype* arch = entity_archetypes_[index];
    if (!arch) return nullptr;

    int comp_idx = arch->component_index(type_id);
    if (comp_idx < 0) return nullptr;

    Chunk* chunk = entity_chunks_[index];
    usize row = entity_rows_[index];
    if (!chunk || row == static_cast<usize>(-1)) return nullptr;

    const ComponentInfo* info = get_component_info(type_id);
    usize comp_size = info ? info->size : 8;
    return chunk->get_component_ptr(row, static_cast<usize>(comp_idx), comp_size);
}

auto Storage::has_component_internal(Entity entity, ComponentTypeId type_id) const -> bool {
    u32 index = entity.index();
    if (index >= entity_generations_.size()) return false;

    Archetype* arch = entity_archetypes_[index];
    if (!arch) return false;

    return arch->has_component(type_id);
}

auto Storage::move_entity(Entity entity, Archetype* from_arch, Archetype* to_arch, ComponentTypeId new_type, const void* new_value, usize new_size) -> void {
    u32 index = entity.index();
    Chunk* old_chunk = entity_chunks_[index];
    usize old_row = entity_rows_[index];

    std::vector<usize> comp_sizes;
    for (auto cid : to_arch->type_mask()) {
        const ComponentInfo* info = get_component_info(cid);
        comp_sizes.push_back(info ? info->size : 8);
    }

    Chunk* new_chunk = to_arch->chunks_;
    while (new_chunk && new_chunk->is_full()) {
        new_chunk = new_chunk->next;
    }
    if (!new_chunk) {
        new_chunk = chunk_allocator_.allocate(*to_arch, comp_sizes);
        new_chunk->next = to_arch->chunks_;
        to_arch->chunks_ = new_chunk;
        to_arch->chunk_count_++;
    }

    usize new_row = new_chunk->add_entity(entity);

    for (usize ci = 0; ci < to_arch->type_mask().size(); ++ci) {
        ComponentTypeId cid = to_arch->type_mask()[ci];
        void* dst = new_chunk->get_component_ptr(new_row, ci, comp_sizes[ci]);
        if (!dst) continue;

        if (cid == new_type) {
            std::memcpy(dst, new_value, new_size);
        } else {
            int src_ci = from_arch->component_index(cid);
            if (src_ci >= 0) {
                usize src_size = 0;
                for (usize si = 0; si < from_arch->type_mask().size(); ++si) {
                    if (from_arch->type_mask()[si] == cid) {
                        const ComponentInfo* info = get_component_info(cid);
                        src_size = info ? info->size : 8;
                        break;
                    }
                }
                const void* src = old_chunk->get_component_ptr(old_row, static_cast<usize>(src_ci), src_size);
                if (src) {
                    std::memcpy(dst, src, comp_sizes[ci] < src_size ? comp_sizes[ci] : src_size);
                }
            }
        }
    }

    if (old_chunk) {
        old_chunk->remove_entity(old_row);
    }

    entity_archetypes_[index] = to_arch;
    entity_chunks_[index] = new_chunk;
    entity_rows_[index] = new_row;
}

auto Storage::move_entity_remove(Entity entity, Archetype* from_arch, Archetype* to_arch, ComponentTypeId /*removed_type*/) -> void {
    u32 index = entity.index();
    Chunk* old_chunk = entity_chunks_[index];
    usize old_row = entity_rows_[index];

    std::vector<usize> comp_sizes;
    for (auto cid : to_arch->type_mask()) {
        const ComponentInfo* info = get_component_info(cid);
        comp_sizes.push_back(info ? info->size : 8);
    }

    Chunk* new_chunk = to_arch->chunks_;
    while (new_chunk && new_chunk->is_full()) {
        new_chunk = new_chunk->next;
    }
    if (!new_chunk) {
        new_chunk = chunk_allocator_.allocate(*to_arch, comp_sizes);
        new_chunk->next = to_arch->chunks_;
        to_arch->chunks_ = new_chunk;
        to_arch->chunk_count_++;
    }

    usize new_row = new_chunk->add_entity(entity);

    for (usize ci = 0; ci < to_arch->type_mask().size(); ++ci) {
        ComponentTypeId cid = to_arch->type_mask()[ci];
        void* dst = new_chunk->get_component_ptr(new_row, ci, comp_sizes[ci]);
        if (!dst) continue;

        int src_ci = from_arch->component_index(cid);
        if (src_ci >= 0) {
            usize src_size = 0;
            for (usize si = 0; si < from_arch->type_mask().size(); ++si) {
                if (from_arch->type_mask()[si] == cid) {
                    const ComponentInfo* info = get_component_info(cid);
                    src_size = info ? info->size : 8;
                    break;
                }
            }
            const void* src = old_chunk->get_component_ptr(old_row, static_cast<usize>(src_ci), src_size);
            if (src) {
                std::memcpy(dst, src, comp_sizes[ci] < src_size ? comp_sizes[ci] : src_size);
            }
        }
    }

    if (old_chunk) {
        old_chunk->remove_entity(old_row);
    }

    entity_archetypes_[index] = to_arch;
    entity_chunks_[index] = new_chunk;
    entity_rows_[index] = new_row;
}

} // namespace okn::ecs
