#include <okn/ecs/scheduler/scheduler.hpp>
#include <okn/ecs/scheduler/system_graph.hpp>
#include <okn/ecs/scheduler/job_adapter.hpp>
#include <okn/ecs/world.hpp>

namespace okn::ecs {

Scheduler::Scheduler(SystemGraph& graph)
    : graph_(&graph) {
}

void Scheduler::run(World& world, float delta_time) {
    if (order_dirty_) {
        rebuild_order();
    }

    for (auto* sys : execution_order_) {
        if (sys) {
            sys->execute(world, delta_time);
        }
    }
}

void Scheduler::set_job_system(IJobSystem* job_system) {
    job_system_ = job_system;
}

void Scheduler::invalidate_order() {
    order_dirty_ = true;
}

void Scheduler::rebuild_order() {
    execution_order_ = graph_->build_execution_order();
    order_dirty_ = false;
}

} // namespace okn::ecs
