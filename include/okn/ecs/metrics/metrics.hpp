#pragma once

#include <okn/ecs/ecs_types.hpp>

namespace okn::ecs {

class World;
class SystemGraph;

struct EcsMetrics {
    usize entity_count = 0;
    usize component_count = 0;
    usize archetype_count = 0;
    usize chunk_count = 0;
    float system_time_ms[16] = {};
    usize system_names_count = 0;
    const char* system_names[16] = {};
};

class EcsDebugView {
public:
    static auto collect(const World& world, const SystemGraph* graph = nullptr) -> EcsMetrics;
};

} // namespace okn::ecs
