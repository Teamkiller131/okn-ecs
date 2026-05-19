#include <doctest/doctest.h>
#include <okn/ecs/hierarchy/hierarchy.hpp>
#include <okn/ecs/hierarchy/transform.hpp>
#include <okn/ecs/world.hpp>
#include <cmath>

using namespace okn::ecs;

TEST_CASE("LocalTransform - default values") {
    LocalTransform lt;
    CHECK(lt.pos[0] == 0.0f);
    CHECK(lt.pos[1] == 0.0f);
    CHECK(lt.pos[2] == 0.0f);
    CHECK(lt.rot[0] == 0.0f);
    CHECK(lt.rot[1] == 0.0f);
    CHECK(lt.rot[2] == 0.0f);
    CHECK(lt.rot[3] == 1.0f);
    CHECK(lt.scl[0] == 1.0f);
    CHECK(lt.scl[1] == 1.0f);
    CHECK(lt.scl[2] == 1.0f);
}

TEST_CASE("WorldTransform - default values") {
    WorldTransform wt;
    CHECK(wt.pos[0] == 0.0f);
    CHECK(wt.pos[1] == 0.0f);
    CHECK(wt.pos[2] == 0.0f);
    CHECK(wt.rot[3] == 1.0f);
    CHECK(wt.scl[0] == 1.0f);
    CHECK(wt.mat[0] == 1.0f);
    CHECK(wt.mat[5] == 1.0f);
    CHECK(wt.mat[10]== 1.0f);
    CHECK(wt.mat[15]== 1.0f);
}

TEST_CASE("compute_world_matrix - identity") {
    LocalTransform local;
    auto wt = compute_world_matrix(local);
    CHECK(wt.pos[0] == 0.0f);
    CHECK(wt.pos[1] == 0.0f);
    CHECK(wt.pos[2] == 0.0f);
    CHECK(wt.mat[0] == 1.0f);
    CHECK(wt.mat[5] == 1.0f);
    CHECK(wt.mat[10]== 1.0f);
    CHECK(wt.mat[15]== 1.0f);
}

TEST_CASE("compute_world_matrix - translation") {
    LocalTransform local;
    local.pos[0] = 10.0f;
    local.pos[1] = 20.0f;
    local.pos[2] = 30.0f;

    auto wt = compute_world_matrix(local);
    CHECK(wt.pos[0] == 10.0f);
    CHECK(wt.pos[1] == 20.0f);
    CHECK(wt.pos[2] == 30.0f);
    CHECK(wt.mat[12]== 10.0f);
    CHECK(wt.mat[13]== 20.0f);
    CHECK(wt.mat[14]== 30.0f);
}

TEST_CASE("compute_world_matrix - with parent") {
    LocalTransform parent_local;
    parent_local.pos[0] = 5.0f;
    parent_local.pos[1] = 0.0f;
    parent_local.pos[2] = 0.0f;

    auto parent_wt = compute_world_matrix(parent_local);

    LocalTransform child_local;
    child_local.pos[0] = 3.0f;
    child_local.pos[1] = 0.0f;
    child_local.pos[2] = 0.0f;

    auto child_wt = compute_world_matrix(child_local, parent_wt.mat);
    CHECK(child_wt.pos[0] == 8.0f);
    CHECK(child_wt.pos[1] == 0.0f);
    CHECK(child_wt.pos[2] == 0.0f);
}

TEST_CASE("compute_world_matrix - scale") {
    LocalTransform local;
    local.scl[0] = 2.0f;
    local.scl[1] = 3.0f;
    local.scl[2] = 4.0f;

    auto wt = compute_world_matrix(local);
    CHECK(wt.scl[0] == 2.0f);
    CHECK(wt.scl[1] == 3.0f);
    CHECK(wt.scl[2] == 4.0f);
}

TEST_CASE("decompose_matrix - identity") {
    float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    auto wt = decompose_matrix(identity);
    CHECK(wt.pos[0] == 0.0f);
    CHECK(wt.pos[1] == 0.0f);
    CHECK(wt.pos[2] == 0.0f);
    CHECK(wt.scl[0] == 1.0f);
    CHECK(wt.scl[1] == 1.0f);
    CHECK(wt.scl[2] == 1.0f);
}

TEST_CASE("decompose_matrix - translation") {
    float mat[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        10,20,30, 1
    };
    auto wt = decompose_matrix(mat);
    CHECK(wt.pos[0] == 10.0f);
    CHECK(wt.pos[1] == 20.0f);
    CHECK(wt.pos[2] == 30.0f);
}

TEST_CASE("Hierarchy - set and get parent") {
    Hierarchy hierarchy;
    World world;
    auto parent = world.create_entity();
    auto child = world.create_entity();

    hierarchy.set_parent(child, parent);
    CHECK(hierarchy.get_parent(child) == parent);
    CHECK(hierarchy.get_parent(parent) == kInvalidEntity);
}

TEST_CASE("Hierarchy - get children") {
    Hierarchy hierarchy;
    World world;
    auto parent = world.create_entity();
    auto child1 = world.create_entity();
    auto child2 = world.create_entity();

    hierarchy.set_parent(child1, parent);
    hierarchy.set_parent(child2, parent);

    const auto& children = hierarchy.get_children(parent);
    CHECK(children.size() == 2);

    const auto& no_children = hierarchy.get_children(child1);
    CHECK(no_children.empty());
}

TEST_CASE("Hierarchy - change parent") {
    Hierarchy hierarchy;
    World world;
    auto parent1 = world.create_entity();
    auto parent2 = world.create_entity();
    auto child = world.create_entity();

    hierarchy.set_parent(child, parent1);
    CHECK(hierarchy.get_parent(child) == parent1);

    hierarchy.set_parent(child, parent2);
    CHECK(hierarchy.get_parent(child) == parent2);
    CHECK(hierarchy.get_children(parent1).empty());
    CHECK(hierarchy.get_children(parent2).size() == 1);
}

TEST_CASE("Hierarchy - remove child") {
    Hierarchy hierarchy;
    World world;
    auto parent = world.create_entity();
    auto child1 = world.create_entity();
    auto child2 = world.create_entity();

    hierarchy.set_parent(child1, parent);
    hierarchy.set_parent(child2, parent);

    hierarchy.remove_child(child1);
    CHECK(hierarchy.get_parent(child1) == kInvalidEntity);
    CHECK(hierarchy.get_children(parent).size() == 1);
}

TEST_CASE("Hierarchy - self-parent is ignored") {
    Hierarchy hierarchy;
    World world;
    auto e = world.create_entity();

    hierarchy.set_parent(e, e);
    CHECK(hierarchy.get_parent(e) == kInvalidEntity);
}

TEST_CASE("Hierarchy - transform update") {
    Hierarchy hierarchy;
    World world;

    auto parent = world.create_entity();
    auto child = world.create_entity();

    LocalTransform parent_local;
    parent_local.pos[0] = 5.0f;
    parent_local.pos[1] = 0.0f;
    parent_local.pos[2] = 0.0f;

    LocalTransform child_local;
    child_local.pos[0] = 3.0f;
    child_local.pos[1] = 0.0f;
    child_local.pos[2] = 0.0f;

    world.add_component(parent, parent_local);
    world.add_component(child, child_local);

    hierarchy.set_parent(child, parent);
    hierarchy.update_transforms(world);

    auto* child_wt = world.get_component<WorldTransform>(child);
    REQUIRE(child_wt != nullptr);
    CHECK(child_wt->pos[0] == 8.0f);
    CHECK(child_wt->pos[1] == 0.0f);
    CHECK(child_wt->pos[2] == 0.0f);
}

TEST_CASE("Hierarchy - dirty tracking") {
    Hierarchy hierarchy;
    World world;
    auto e = world.create_entity();
    world.add_component(e, LocalTransform{});

    hierarchy.update_transforms(world);
    CHECK(hierarchy.dirty_count() == 0);

    hierarchy.set_parent(e, e);
    hierarchy.set_parent(e, e);
}
