#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/scheduler/system.hpp>
#include <vector>

namespace okn::ecs {

class SystemGraph;
class World;

class Scheduler {
public:
    explicit Scheduler(SystemGraph& graph);

    void run(World& world, float delta_time);

    void set_job_system(class IJobSystem* job_system);

    void invalidate_order();

    // Diagnostics / test hooks. level_count() is the number of conflict-free parallel
    // levels in the cached schedule; levelization_count() is how many times that
    // schedule has been (re)built — it should stay flat across frames and only tick
    // up when the system set changes (invalidate_order()).
    [[nodiscard]] auto level_count() const -> usize { return levels_.size(); }
    [[nodiscard]] auto levelization_count() const -> usize { return levelization_count_; }

private:
    SystemGraph* graph_;
    std::vector<System*> execution_order_;
    std::vector<std::vector<System*>> levels_;   // cached conflict-free parallel levels
    usize levelization_count_ = 0;
    bool order_dirty_ = true;
    IJobSystem* job_system_ = nullptr;

    void rebuild_order();
    void rebuild_levels();
    void run_sequential(World& world, float delta_time);
    void run_parallel(World& world, float delta_time);
};

} // namespace okn::ecs
