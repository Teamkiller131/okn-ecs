#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/core/api/types.hpp>
#include <vector>

namespace okn::ecs {

using okn::core::u8;
using okn::core::u16;

class Archetype;

static constexpr usize kChunkSize = 64 * 1024;

class Chunk {
public:
    static constexpr usize kMaxEntitiesPerChunk = 64;

    Chunk(const Archetype& archetype, const std::vector<usize>& component_sizes);
    ~Chunk();

    Chunk(const Chunk&) = delete;
    auto operator=(const Chunk&) -> Chunk& = delete;

    auto entity_count() const -> usize { return entity_count_; }
    auto is_full() const -> bool { return entity_count_ >= kMaxEntitiesPerChunk; }

    auto add_entity(Entity e) -> usize;
    void remove_entity(usize row);

    template <class T>
    auto get_component(usize row, usize column_index) -> T* {
        return reinterpret_cast<T*>(get_component_ptr(row, column_index, sizeof(T)));
    }

    template <class T>
    auto get_component(usize row, usize column_index) const -> const T* {
        return reinterpret_cast<const T*>(get_component_ptr(row, column_index, sizeof(T)));
    }

    auto get_component_ptr(usize row, usize column_index, usize size) -> void*;
    auto get_component_ptr(usize row, usize column_index, usize size) const -> const void*;
    auto entity_at(usize row) const -> Entity;

    Chunk* next = nullptr;

private:
    u8* data_ = nullptr;
    u16* layout_offsets_ = nullptr;
    Entity* entities_ = nullptr;
    usize entity_count_ = 0;
    usize component_count_ = 0;
    u64 free_rows_ = 0;

    auto find_free_row() -> int;
    auto compute_layout(const std::vector<usize>& component_sizes) -> void;
};

} // namespace okn::ecs
