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

    if (job_system_ && !execution_order_.empty()) {
        run_parallel(world, delta_time);
    } else {
        run_sequential(world, delta_time);
    }
}

void Scheduler::run_sequential(World& world, float delta_time) {
    for (auto* sys : execution_order_) {
        if (sys) {
            sys->execute(world, delta_time);
        }
    }
}

void Scheduler::run_parallel(World& world, float delta_time) {
    // Levels are cached (rebuilt only when the system set changes), so the per-frame
    // cost here is just dispatch — not the O(n^2) conflict grouping it used to redo
    // every frame.
    for (auto& level : levels_) {
        if (level.size() == 1) {
            level[0]->execute(world, delta_time);   // no pool overhead for a lone system
        } else {
            for (auto* sys : level) {
                job_system_->submit([sys, &world, delta_time]() {
                    sys->execute(world, delta_time);
                });
            }
            job_system_->wait_all();   // efficient CV-backed join, not a core-burning spin
        }
    }
}

// Group systems into levels where no two systems in the same level have a read/write
// conflict — the parallelizable schedule. Pure function of the execution order + the
// conflict graph, so it is computed once per (re)build and reused every frame.
void Scheduler::rebuild_levels() {
    levels_.clear();
    const usize sys_count = execution_order_.size();
    std::vector<bool> scheduled(sys_count, false);

    for (usize i = 0; i < sys_count; ++i) {
        if (scheduled[i]) continue;

        std::vector<System*> level;
        level.push_back(execution_order_[i]);
        scheduled[i] = true;

        for (usize j = i + 1; j < sys_count; ++j) {
            if (scheduled[j]) continue;

            bool conflict = false;
            for (auto* s : level) {
                if (graph_->has_conflict(*s, *execution_order_[j])) {
                    conflict = true;
                    break;
                }
            }
            if (!conflict) {
                level.push_back(execution_order_[j]);
                scheduled[j] = true;
            }
        }
        levels_.push_back(std::move(level));
    }
    ++levelization_count_;
}

void Scheduler::set_job_system(IJobSystem* job_system) {
    job_system_ = job_system;
}

void Scheduler::invalidate_order() {
    order_dirty_ = true;
}

void Scheduler::rebuild_order() {
    execution_order_ = graph_->build_execution_order();
    rebuild_levels();
    order_dirty_ = false;
}

} // namespace okn::ecs
