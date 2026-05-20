#include <doctest/doctest.h>
#include <okn/ecs/archetype/chunk.hpp>
#include <okn/ecs/archetype/archetype.hpp>

using okn::ecs::usize;

TEST_CASE("Chunk - construction with components") {
    std::vector<okn::ecs::ComponentTypeId> comps = {1, 2};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {sizeof(int), sizeof(float)};

    okn::ecs::Chunk chunk(arch, sizes);
    CHECK(chunk.entity_count() == 0);
    CHECK_FALSE(chunk.is_full());
}

TEST_CASE("Chunk - add and access entities") {
    std::vector<okn::ecs::ComponentTypeId> comps = {1, 2};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {sizeof(int), sizeof(float)};

    okn::ecs::Chunk chunk(arch, sizes);

    auto e1 = okn::ecs::Entity(0, 0);
    auto e2 = okn::ecs::Entity(1, 0);

    usize row0 = chunk.add_entity(e1);
    usize row1 = chunk.add_entity(e2);

    CHECK(row0 != static_cast<okn::ecs::usize>(-1));
    CHECK(row1 != static_cast<okn::ecs::usize>(-1));
    CHECK(row0 != row1);
    CHECK(chunk.entity_count() == 2);
    CHECK(chunk.entity_at(row0) == e1);
    CHECK(chunk.entity_at(row1) == e2);
}

TEST_CASE("Chunk - component access") {
    std::vector<okn::ecs::ComponentTypeId> comps = {1, 2};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {sizeof(int), sizeof(float)};

    okn::ecs::Chunk chunk(arch, sizes);

    auto e1 = okn::ecs::Entity(0, 0);
    usize row = chunk.add_entity(e1);

    int* int_ptr = chunk.get_component<int>(row, 0);
    float* float_ptr = chunk.get_component<float>(row, 1);

    CHECK(int_ptr != nullptr);
    CHECK(float_ptr != nullptr);

    *int_ptr = 42;
    *float_ptr = 3.14f;

    CHECK(*chunk.get_component<int>(row, 0) == 42);
    CHECK(*chunk.get_component<float>(row, 1) == 3.14f);
}

TEST_CASE("Chunk - const component access") {
    std::vector<okn::ecs::ComponentTypeId> comps = {1};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {sizeof(int)};

    okn::ecs::Chunk chunk(arch, sizes);
    auto e1 = okn::ecs::Entity(0, 0);
    usize row = chunk.add_entity(e1);
    *chunk.get_component<int>(row, 0) = 100;

    const okn::ecs::Chunk& cchunk = chunk;
    CHECK(*cchunk.get_component<int>(row, 0) == 100);
}

TEST_CASE("Chunk - remove entity") {
    std::vector<okn::ecs::ComponentTypeId> comps = {1};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {sizeof(int)};

    okn::ecs::Chunk chunk(arch, sizes);

    auto e1 = okn::ecs::Entity(0, 0);
    auto e2 = okn::ecs::Entity(1, 0);
    auto e3 = okn::ecs::Entity(2, 0);

    chunk.add_entity(e1);
    usize row2 = chunk.add_entity(e2);
    chunk.add_entity(e3);

    CHECK(chunk.entity_count() == 3);
    chunk.remove_entity(row2);
    CHECK(chunk.entity_count() == 2);
}

TEST_CASE("Chunk - fill to capacity") {
    std::vector<okn::ecs::ComponentTypeId> comps = {1};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {sizeof(int)};

    okn::ecs::Chunk chunk(arch, sizes);

    for (okn::ecs::usize i = 0; i < okn::ecs::Chunk::kMaxEntitiesPerChunk; ++i) {
        auto e = okn::ecs::Entity(static_cast<okn::ecs::u32>(i), 0);
        usize row = chunk.add_entity(e);
        CHECK(row != static_cast<okn::ecs::usize>(-1));
    }

    CHECK(chunk.entity_count() == okn::ecs::Chunk::kMaxEntitiesPerChunk);
    CHECK(chunk.is_full());
}

TEST_CASE("Chunk - null component access out of bounds") {
    std::vector<okn::ecs::ComponentTypeId> comps = {1};
    okn::ecs::Archetype arch(comps);
    std::vector<okn::ecs::usize> sizes = {sizeof(int)};

    okn::ecs::Chunk chunk(arch, sizes);
    // chunk component access implementation-specific
    CHECK(chunk.get_component_ptr(0, 999, 4) == nullptr);
    CHECK(chunk.get_component_ptr(9999, 0, 4) == nullptr);
}