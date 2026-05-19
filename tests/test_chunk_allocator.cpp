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

TEST_CASE("ChunkAllocator - reset clears all") {
    okn::ecs::ChunkAllocator allocator;
    std::vector<okn::ecs::ComponentTypeId> comps = {1};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {8};

    auto* c1 = allocator.allocate(arch, sizes);
    auto* c2 = allocator.allocate(arch, sizes);
    allocator.deallocate(c1);
    allocator.deallocate(c2);

    allocator.reset();

    auto* c3 = allocator.allocate(arch, sizes);
    CHECK(c3 != c1);
    CHECK(c3 != c2);
    allocator.deallocate(c3);
}
