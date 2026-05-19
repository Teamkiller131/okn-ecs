#include <okn/ecs/scheduler/scheduler.hpp>
#include <okn/ecs/scheduler/system_graph.hpp>
#include <okn/ecs/scheduler/job_adapter.hpp>
#include <okn/ecs/world.hpp>
#include <atomic>

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
    const auto sys_count = execution_order_.size();
    if (sys_count == 0) return;

    // Group systems into levels where no two systems in the same level
    // have a read/write conflict with each other.
    std::vector<std::vector<System*>> levels;
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
        levels.push_back(std::move(level));
    }

    for (auto& level : levels) {
        if (level.size() == 1) {
            level[0]->execute(world, delta_time);
        } else {
            std::atomic<usize> pending{level.size()};
            for (auto* sys : level) {
                job_system_->submit([sys, &world, delta_time, &pending]() {
                    sys->execute(world, delta_time);
                    pending.fetch_sub(1, std::memory_order_release);
                });
            }
            while (pending.load(std::memory_order_acquire) > 0) {
                // spin-wait for all jobs in this level to complete
            }
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
