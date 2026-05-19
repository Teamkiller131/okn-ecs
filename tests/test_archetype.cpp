#include <doctest/doctest.h>
#include <okn/ecs/archetype/archetype.hpp>

TEST_CASE("Archetype - empty construction") {
    okn::ecs::Archetype arch;
    CHECK(arch.component_count() == 0);
    CHECK(arch.type_mask().empty());
}

TEST_CASE("Archetype - construction with components") {
    std::vector<okn::ecs::ComponentTypeId> comps = {3, 1, 2};
    okn::ecs::Archetype arch(comps);
    CHECK(arch.component_count() == 3);
    const auto& mask = arch.type_mask();
    CHECK(mask[0] == 1);
    CHECK(mask[1] == 2);
    CHECK(mask[2] == 3);
}

TEST_CASE("Archetype - has_component") {
    std::vector<okn::ecs::ComponentTypeId> comps = {1, 3, 5};
    okn::ecs::Archetype arch(comps);
    CHECK(arch.has_component(1));
    CHECK(arch.has_component(3));
    CHECK(arch.has_component(5));
    CHECK_FALSE(arch.has_component(0));
    CHECK_FALSE(arch.has_component(2));
    CHECK_FALSE(arch.has_component(4));
}

TEST_CASE("Archetype - component_index") {
    std::vector<okn::ecs::ComponentTypeId> comps = {10, 20, 30};
    okn::ecs::Archetype arch(comps);
    CHECK(arch.component_index(10) == 0);
    CHECK(arch.component_index(20) == 1);
    CHECK(arch.component_index(30) == 2);
    CHECK(arch.component_index(0) == -1);
    CHECK(arch.component_index(15) == -1);
    CHECK(arch.component_index(40) == -1);
}

TEST_CASE("Archetype - edge management") {
    std::vector<okn::ecs::ComponentTypeId> comps = {1, 2};
    okn::ecs::Archetype arch(comps);

    auto& edge1 = arch.add_edge(3);
    CHECK(edge1.component == 3);
    CHECK(edge1.archetype == nullptr);

    auto& edge2 = arch.add_edge(3);
    CHECK(&edge2 == &edge1);

    auto& edge3 = arch.add_edge(4);
    CHECK(edge3.component == 4);
    CHECK(arch.edges().size() == 2);

    const okn::ecs::Archetype* carch = &arch;
    const auto* e1 = carch->get_edge(3);
    CHECK(e1 != nullptr);
    CHECK(e1->component == 3);

    const auto* e2 = carch->get_edge(5);
    CHECK(e2 == nullptr);
}
