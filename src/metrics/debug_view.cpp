#include <okn/ecs/metrics/metrics.hpp>
#include <okn/ecs/world.hpp>
#include <okn/ecs/scheduler/system_graph.hpp>

namespace okn::ecs {

auto EcsDebugView::collect(const World& world, const SystemGraph* graph) -> EcsMetrics {
    EcsMetrics metrics{};
    metrics.entity_count = world.entity_count();

    if (graph != nullptr) {
        metrics.system_names_count = 0;
        auto order = const_cast<SystemGraph*>(graph)->build_execution_order();
        for (usize i = 0; i < order.size() && metrics.system_names_count < 16; ++i) {
            if (order[i]) {
                metrics.system_names[metrics.system_names_count] = order[i]->name().c_str();
                metrics.system_names_count++;
            }
        }
    }

    return metrics;
}

} // namespace okn::ecs
