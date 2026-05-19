#include <doctest/doctest.h>
#include <okn/ecs/scheduler/system_graph.hpp>
#include <okn/ecs/world.hpp>

namespace okn::ecs {
namespace {

struct Position { float x = 0; float y = 0; float z = 0; };
struct Velocity { float dx = 0; float dy = 0; float dz = 0; };
struct Health   { int hp = 100; };
struct Transform { float tx = 0; float ty = 0; float tz = 0; };

class SysA : public System {
public:
    SysA() { set_name("SysA"); }
    auto reads() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Position>()};
    }
    auto writes() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Velocity>()};
    }
    void execute(World&, float) override {}
};

class SysB : public System {
public:
    SysB() { set_name("SysB"); }
    auto reads() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Velocity>()};
    }
    auto writes() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Position>()};
    }
    void execute(World&, float) override {}
};

class SysC : public System {
public:
    SysC() { set_name("SysC"); }
    auto reads() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Health>()};
    }
    auto writes() const -> std::vector<ComponentTypeId> override {
        return {World::component_type_id<Health>()};
    }
    void execute(World&, float) override {}
};

class SysSequentialA : public System {
public:
    SysSequentialA() { set_name("SequentialA"); }
    auto after() const -> std::vector<std::string> override {
        return {"SequentialB"};
    }
    void execute(World&, float) override {}
};

class SysSequentialB : public System {
public:
    SysSequentialB() { set_name("SequentialB"); }
    auto before() const -> std::vector<std::string> override {
        return {"SequentialC"};
    }
    void execute(World&, float) override {}
};

class SysSequentialC : public System {
public:
    SysSequentialC() { set_name("SequentialC"); }
    void execute(World&, float) override {}
};

} // namespace
} // namespace okn::ecs

using namespace okn::ecs;

TEST_CASE("SystemGraph - add and get systems") {
    SystemGraph graph;

    SUBCASE("add system returns via get") {
        auto sys = std::make_unique<SysA>();
        graph.add_system(std::move(sys));
        auto* found = graph.get_system("SysA");
        CHECK(found != nullptr);
        CHECK(found->name() == "SysA");
    }

    SUBCASE("get non-existent returns nullptr") {
        CHECK(graph.get_system("NoSuchSystem") == nullptr);
    }

    SUBCASE("duplicate name is ignored") {
        auto sys1 = std::make_unique<SysA>();
        auto sys2 = std::make_unique<SysA>();
        graph.add_system(std::move(sys1));
        graph.add_system(std::move(sys2));
        CHECK(graph.system_count() == 1);
    }
}

TEST_CASE("SystemGraph - has_conflict detection") {
    SysA a;
    SysB b;
    SysC c;

    SystemGraph graph;
    CHECK(graph.has_conflict(a, b));
    CHECK(graph.has_conflict(b, a));
    CHECK(!graph.has_conflict(a, c));
}

TEST_CASE("SystemGraph - topological sort") {
    SUBCASE("empty graph returns empty order") {
        SystemGraph graph;
        auto order = graph.build_execution_order();
        CHECK(order.empty());
    }

    SUBCASE("single system") {
        SystemGraph graph;
        graph.add_system(std::make_unique<SysA>());
        auto order = graph.build_execution_order();
        CHECK(order.size() == 1);
        CHECK(order[0]->name() == "SysA");
    }

    SUBCASE("non-conflicting systems") {
        SystemGraph graph;
        graph.add_system(std::make_unique<SysA>());
        graph.add_system(std::make_unique<SysC>());
        auto order = graph.build_execution_order();
        CHECK(order.size() == 2);
    }

    SUBCASE("conflicting systems are ordered") {
        SystemGraph graph;
        graph.add_system(std::make_unique<SysA>());
        graph.add_system(std::make_unique<SysB>());
        auto order = graph.build_execution_order();
        CHECK(order.size() == 2);
    }

    SUBCASE("explicit before/after ordering") {
        SystemGraph graph;
        graph.add_system(std::make_unique<SysSequentialA>());
        graph.add_system(std::make_unique<SysSequentialB>());
        graph.add_system(std::make_unique<SysSequentialC>());

        auto order = graph.build_execution_order();
        CHECK(order.size() == 3);

        int idx_b = -1, idx_c = -1, idx_a = -1;
        for (int i = 0; i < 3; ++i) {
            if (order[i]->name() == "SequentialA") idx_a = i;
            if (order[i]->name() == "SequentialB") idx_b = i;
            if (order[i]->name() == "SequentialC") idx_c = i;
        }

        CHECK(idx_a < idx_b);
        CHECK(idx_c < idx_b);
    }
}
