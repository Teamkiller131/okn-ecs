#include <okn/ecs/world.hpp>
#include <algorithm>

namespace okn::ecs {

World::World() = default;

World::~World() = default;

World::World(World&&) noexcept = default;

auto World::operator=(World&&) noexcept -> World& = default;

auto World::create_entity() -> Entity {
    u32 idx;
    u32 gen;

    if (!free_entity_indices_.empty()) {
        idx = free_entity_indices_.back();
        free_entity_indices_.pop_back();
        gen = entity_generations_[idx] + 1;
        entity_generations_[idx] = gen;
    } else {
        idx = next_entity_index_++;
        gen = 1;
        if (idx >= entity_generations_.size()) {
            entity_generations_.resize(idx + 1, 0);
        }
        entity_generations_[idx] = gen;
    }

    ++alive_count_;
    return Entity(idx, gen);
}

void World::destroy_entity(Entity entity) {
    if (!entity_alive(entity)) return;

    u32 idx = entity.index();
    remove_all_components(entity);
    ++entity_generations_[idx];
    free_entity_indices_.push_back(idx);
    --alive_count_;
}

auto World::entity_alive(Entity entity) const -> bool {
    if (!entity.is_valid()) return false;
    u32 idx = entity.index();
    if (idx >= entity_generations_.size()) return false;
    return entity_generations_[idx] != 0 && entity_generations_[idx] == entity.generation();
}

auto World::entity_count() const -> usize {
    return alive_count_;
}

void World::ensure_sparse_capacity(ComponentStore& store, u32 entity_index) {
    if (entity_index >= store.sparse.size()) {
        store.sparse.resize(entity_index + 1, ~0u);
    }
}

void World::remove_all_components(Entity entity) {
    for (auto& [cid, store] : stores_) {
        store.remove(entity);
    }
}

// ── ComponentStore methods ──

void World::ComponentStore::push(Entity entity, const void* value) {
    u32 eidx = entity.index();
    if (count >= entities.size()) {
        usize new_cap = count == 0 ? 8 : count * 2;
        entities.resize(new_cap);
        data.resize(new_cap * component_size);
    }
    usize slot = count++;
    entities[slot] = entity;
    std::memcpy(data.data() + slot * component_size, value, component_size);
    sparse[eidx] = static_cast<u32>(slot);
}

void World::ComponentStore::remove(Entity entity) {
    u32 eidx = entity.index();
    if (eidx >= sparse.size()) return;
    u32 slot = sparse[eidx];
    if (slot >= static_cast<u32>(count) || entities[slot] != entity) return;

    usize last = count - 1;
    if (slot != last) {
        std::memcpy(data.data() + slot * component_size,
                    data.data() + last * component_size,
                    component_size);
        entities[slot] = entities[last];
        sparse[entities[last].index()] = slot;
    }
    sparse[eidx] = ~0u;
    --count;
}

auto World::ComponentStore::get(Entity entity) const -> const void* {
    u32 eidx = entity.index();
    if (eidx >= sparse.size()) return nullptr;
    u32 slot = sparse[eidx];
    if (slot >= static_cast<u32>(count) || entities[slot] != entity) return nullptr;
    return data.data() + slot * component_size;
}

auto World::ComponentStore::get(Entity entity) -> void* {
    u32 eidx = entity.index();
    if (eidx >= sparse.size()) return nullptr;
    u32 slot = sparse[eidx];
    if (slot >= static_cast<u32>(count) || entities[slot] != entity) return nullptr;
    return data.data() + slot * component_size;
}

auto World::ComponentStore::has(Entity entity) const -> bool {
    u32 eidx = entity.index();
    if (eidx >= sparse.size()) return false;
    u32 slot = sparse[eidx];
    return slot < static_cast<u32>(count) && entities[slot] == entity;
}

} // namespace okn::ecs
