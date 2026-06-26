#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/scheduler/system.hpp>
#include <vector>

// The job system is okn-platform's single IJobSystem (work-stealing pool); the ECS
// scheduler no longer ships its own duplicate interface/pool.
namespace okn::platform { class IJobSystem; }

namespace okn::ecs {

class SystemGraph;
class World;

class Scheduler {
public:
    explicit Scheduler(SystemGraph& graph);

    void run(World& world, float delta_time);

    // Set the work-stealing pool the parallel path runs on (null => sequential).
    // INVARIANT: the scheduler joins each parallel level by draining the pool to
    // quiescence (wait_all), so it requires EXCLUSIVE use of this pool while run() is
    // executing. Don't submit to the same pool from another producer concurrently, or
    // the per-level barrier will over-synchronize and race the job accounting — give
    // the scheduler its own pool instance.
    void set_job_system(okn::platform::IJobSystem* job_system);

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
    okn::platform::IJobSystem* job_system_ = nullptr;

    void rebuild_order();
    void rebuild_levels();
    void run_sequential(World& world, float delta_time);
    void run_parallel(World& world, float delta_time);
};

} // namespace okn::ecs
