#include <okn/ecs/archetype/chunk_allocator.hpp>
#include <okn/ecs/archetype/archetype.hpp>
#include <new>

namespace okn::ecs {

ChunkAllocator::~ChunkAllocator() {
    reset();
}

auto ChunkAllocator::allocate(const Archetype& archetype, const std::vector<usize>& component_sizes) -> Chunk* {
    if (free_list_) {
        Chunk* chunk = free_list_;
        free_list_ = chunk->next;
        chunk->next = nullptr;
        chunk->~Chunk();
        new (chunk) Chunk(archetype, component_sizes);
        return chunk;
    }
    return new Chunk(archetype, component_sizes);
}

void ChunkAllocator::deallocate(Chunk* chunk) {
    if (!chunk) return;
    chunk->next = free_list_;
    free_list_ = chunk;
}

void ChunkAllocator::reset() {
    while (free_list_) {
        Chunk* next = free_list_->next;
        free_list_->next = nullptr;
        delete free_list_;
        free_list_ = next;
    }
}

} // namespace okn::ecs
