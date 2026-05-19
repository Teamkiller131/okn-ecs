#include <doctest/doctest.h>
#include <okn/ecs/sparse/sparse_set.hpp>

namespace okn::ecs {
namespace {

struct TestPod {
    int x = 0;
    float y = 0.0f;
};

} // namespace
} // namespace okn::ecs

TEST_CASE("SparseSet - insert and get") {
    okn::ecs::SparseSet<int> set;
    auto e1 = okn::ecs::Entity(0, 0);
    auto e2 = okn::ecs::Entity(1, 0);
    auto e3 = okn::ecs::Entity(2, 0);

    set.insert(e1, 10);
    set.insert(e2, 20);
    set.insert(e3, 30);

    CHECK(set.size() == 3);
    CHECK(*set.get(e1) == 10);
    CHECK(*set.get(e2) == 20);
    CHECK(*set.get(e3) == 30);
    CHECK(set.contains(e1));
    CHECK(set.contains(e2));
    CHECK(set.contains(e3));
}

TEST_CASE("SparseSet - insert overwrite") {
    okn::ecs::SparseSet<int> set;
    auto e1 = okn::ecs::Entity(0, 0);

    set.insert(e1, 10);
    CHECK(*set.get(e1) == 10);

    set.insert(e1, 42);
    CHECK(*set.get(e1) == 42);
    CHECK(set.size() == 1);
}

TEST_CASE("SparseSet - remove and swap-pop") {
    okn::ecs::SparseSet<int> set;
    auto e1 = okn::ecs::Entity(0, 0);
    auto e2 = okn::ecs::Entity(1, 0);
    auto e3 = okn::ecs::Entity(2, 0);

    set.insert(e1, 10);
    set.insert(e2, 20);
    set.insert(e3, 30);

    set.remove(e2);
    CHECK(set.size() == 2);
    CHECK(set.contains(e1));
    CHECK_FALSE(set.contains(e2));
    CHECK(set.contains(e3));
    CHECK(*set.get(e1) == 10);
    CHECK(*set.get(e3) == 30);
}

TEST_CASE("SparseSet - remove non-existent") {
    okn::ecs::SparseSet<int> set;
    auto e1 = okn::ecs::Entity(0, 0);
    auto e2 = okn::ecs::Entity(1, 0);

    set.insert(e1, 10);
    set.remove(e2);
    CHECK(set.size() == 1);
    CHECK(set.contains(e1));
}

TEST_CASE("SparseSet - clear") {
    okn::ecs::SparseSet<int> set;
    auto e1 = okn::ecs::Entity(0, 0);
    auto e2 = okn::ecs::Entity(1, 0);

    set.insert(e1, 10);
    set.insert(e2, 20);
    set.clear();

    CHECK(set.size() == 0);
    CHECK_FALSE(set.contains(e1));
    CHECK_FALSE(set.contains(e2));
}

TEST_CASE("SparseSet - dense iteration") {
    okn::ecs::SparseSet<int> set;
    auto e1 = okn::ecs::Entity(0, 0);
    auto e2 = okn::ecs::Entity(1, 0);
    auto e3 = okn::ecs::Entity(2, 0);

    set.insert(e1, 100);
    set.insert(e2, 200);
    set.insert(e3, 300);

    int sum = 0;
    for (okn::ecs::usize i = 0; i < set.dense_size(); ++i) {
        sum += set.dense()[i];
    }
    CHECK(sum == 600);

    const auto& cset = set;
    int sum2 = 0;
    for (okn::ecs::usize i = 0; i < cset.dense_size(); ++i) {
        sum2 += cset.dense()[i];
    }
    CHECK(sum2 == 600);
}

TEST_CASE("SparseSet - high index entity") {
    okn::ecs::SparseSet<int> set;
    auto e1 = okn::ecs::Entity(10000, 0);
    set.insert(e1, 42);
    CHECK(*set.get(e1) == 42);
    CHECK(set.size() == 1);
}
