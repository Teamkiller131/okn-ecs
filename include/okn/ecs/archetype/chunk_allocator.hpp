#pragma once

#include <okn/ecs/archetype/chunk.hpp>
#include <vector>

namespace okn::ecs {

class ChunkAllocator {
public:
    ChunkAllocator() = default;
    ~ChunkAllocator();

    ChunkAllocator(const ChunkAllocator&) = delete;
    auto operator=(const ChunkAllocator&) -> ChunkAllocator& = delete;

    auto allocate(const Archetype& archetype, const std::vector<usize>& component_sizes) -> Chunk*;
    void deallocate(Chunk* chunk);
    void reset();

private:
    Chunk* free_list_ = nullptr;
};

} // namespace okn::ecs
