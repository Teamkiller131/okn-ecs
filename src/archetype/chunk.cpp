#include <okn/ecs/archetype/chunk.hpp>
#include <okn/ecs/archetype/archetype.hpp>
#include <cstring>
#include <cstdlib>
#include <new>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace okn::ecs {

Chunk::Chunk(const Archetype& archetype, const std::vector<usize>& component_sizes)
    : component_count_(archetype.component_count()) {
#ifdef _WIN32
    data_ = static_cast<u8*>(_aligned_malloc(kChunkSize, alignof(std::max_align_t)));
#else
    data_ = static_cast<u8*>(::operator new(kChunkSize, std::align_val_t{alignof(std::max_align_t)}));
#endif
    std::memset(data_, 0, kChunkSize);

    compute_layout(component_sizes);

    entities_ = reinterpret_cast<Entity*>(data_);
    entity_count_ = 0;
    free_rows_ = 0;
}

Chunk::~Chunk() {
    if (data_) {
#ifdef _WIN32
        _aligned_free(data_);
#else
        ::operator delete(data_, std::align_val_t{alignof(std::max_align_t)});
#endif
        data_ = nullptr;
    }
    delete[] layout_offsets_;
    layout_offsets_ = nullptr;
}

void Chunk::compute_layout(const std::vector<usize>& component_sizes) {
    if (component_count_ == 0) {
        layout_offsets_ = nullptr;
        return;
    }
    layout_offsets_ = new u16[component_count_];

    usize offset = kMaxEntitiesPerChunk * sizeof(Entity);
    for (usize i = 0; i < component_count_; ++i) {
        usize align = component_sizes[i];
        if (align > 16) align = 16;
        offset = (offset + align - 1) & ~(align - 1);
        layout_offsets_[i] = static_cast<u16>(offset);
        offset += kMaxEntitiesPerChunk * component_sizes[i];
    }
}

auto Chunk::find_free_row() -> int {
    for (usize i = 0; i < kMaxEntitiesPerChunk; ++i) {
        if ((free_rows_ & (u64{1} << i)) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

auto Chunk::add_entity(Entity e) -> usize {
    int row = find_free_row();
    if (row < 0) {
        return static_cast<usize>(-1);
    }
    free_rows_ |= (u64{1} << static_cast<usize>(row));
    entities_[static_cast<usize>(row)] = e;
    ++entity_count_;
    return static_cast<usize>(row);
}

void Chunk::remove_entity(usize row) {
    if (row >= kMaxEntitiesPerChunk) return;
    if ((free_rows_ & (u64{1} << row)) == 0) return;

    free_rows_ &= ~(u64{1} << row);
    entities_[row] = Entity{};
    --entity_count_;

    usize last = kMaxEntitiesPerChunk - 1;
    while (last > 0 && (free_rows_ & (u64{1} << last)) == 0) {
        --last;
    }
}

auto Chunk::get_component_ptr(usize row, usize column_index, usize size) -> void* {
    if (column_index >= component_count_ || row >= kMaxEntitiesPerChunk) {
        return nullptr;
    }
    return data_ + layout_offsets_[column_index] + row * size;
}

auto Chunk::get_component_ptr(usize row, usize column_index, usize size) const -> const void* {
    if (column_index >= component_count_ || row >= kMaxEntitiesPerChunk) {
        return nullptr;
    }
    return data_ + layout_offsets_[column_index] + row * size;
}

auto Chunk::entity_at(usize row) const -> Entity {
    if (row >= kMaxEntitiesPerChunk) return Entity{};
    return entities_[row];
}

} // namespace okn::ecs
