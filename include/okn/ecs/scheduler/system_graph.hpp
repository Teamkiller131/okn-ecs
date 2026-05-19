#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/scheduler/system.hpp>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace okn::ecs {

class SystemGraph {
public:
    void add_system(std::unique_ptr<System> system);
    auto get_system(const std::string& name) -> System*;

    auto build_execution_order() -> std::vector<System*>;

    auto has_conflict(const System& a, const System& b) const -> bool;

    auto system_count() const -> usize { return systems_.size(); }

private:
    std::vector<std::unique_ptr<System>> systems_;
    std::unordered_map<std::string, usize> name_index_;

    auto resolve_name_index(const std::string& name) const -> usize;
};

} // namespace okn::ecs
