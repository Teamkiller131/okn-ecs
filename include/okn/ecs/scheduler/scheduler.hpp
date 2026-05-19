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

private:
    SystemGraph* graph_;
    std::vector<System*> execution_order_;
    bool order_dirty_ = true;
    IJobSystem* job_system_ = nullptr;

    void rebuild_order();
    void run_sequential(World& world, float delta_time);
    void run_parallel(World& world, float delta_time);
};

} // namespace okn::ecs
