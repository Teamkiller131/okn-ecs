#include <doctest/doctest.h>
#include <okn/ecs/archetype/chunk_allocator.hpp>
#include <okn/ecs/archetype/archetype.hpp>

TEST_CASE("ChunkAllocator - allocate fresh chunk") {
    okn::ecs::ChunkAllocator allocator;
    std::vector<okn::ecs::ComponentTypeId> comps = {1};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {8};

    auto* chunk = allocator.allocate(arch, sizes);
    CHECK(chunk != nullptr);
    CHECK(chunk->entity_count() == 0);

    allocator.deallocate(chunk);
}

TEST_CASE("ChunkAllocator - reuse freed chunk") {
    okn::ecs::ChunkAllocator allocator;
    std::vector<okn::ecs::ComponentTypeId> comps = {1};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {8};

    auto* c1 = allocator.allocate(arch, sizes);
    auto e = okn::ecs::Entity(0, 0);
    c1->add_entity(e);

    void* addr1 = static_cast<void*>(c1);
    allocator.deallocate(c1);

    auto* c2 = allocator.allocate(arch, sizes);
    CHECK(static_cast<void*>(c2) == addr1);
    CHECK(c2->entity_count() == 0);

    allocator.deallocate(c2);
}

TEST_CASE("ChunkAllocator - multiple allocations") {
    okn::ecs::ChunkAllocator allocator;
    std::vector<okn::ecs::ComponentTypeId> comps = {1, 2};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {8, 4};

    auto* c1 = allocator.allocate(arch, sizes);
    auto* c2 = allocator.allocate(arch, sizes);
    auto* c3 = allocator.allocate(arch, sizes);

    CHECK(c1 != nullptr);
    CHECK(c2 != nullptr);
    CHECK(c3 != nullptr);
    CHECK(c1 != c2);
    CHECK(c2 != c3);
    CHECK(c1 != c3);

    allocator.deallocate(c1);
    allocator.deallocate(c2);
    allocator.deallocate(c3);
}

TEST_CASE("ChunkAllocator - reset clears all and stays usable") {
    okn::ecs::ChunkAllocator allocator;
    std::vector<okn::ecs::ComponentTypeId> comps = {1};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {8};

    auto* c1 = allocator.allocate(arch, sizes);
    auto* c2 = allocator.allocate(arch, sizes);
    REQUIRE(c1 != nullptr);
    REQUIRE(c2 != nullptr);

    // reset() frees every chunk (live or not), unlike deallocate() which only
    // returns a chunk to the free list.
    allocator.reset();

    // The allocator is still usable afterward. We deliberately do NOT compare c3
    // against c1/c2: those were freed by reset(), so the heap is free to hand the
    // same address back — comparing freed pointers is nondeterministic (this was
    // a flaky assertion).
    auto* c3 = allocator.allocate(arch, sizes);
    CHECK(c3 != nullptr);
    allocator.deallocate(c3);
}
