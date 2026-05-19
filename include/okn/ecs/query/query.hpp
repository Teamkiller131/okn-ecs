#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/query/filter.hpp>
#include <vector>

namespace okn::ecs {

class World;

class Query {
public:
    Query(const World& world, const Filter& filter);

    auto entities() const -> const std::vector<Entity>&;
    auto entity_count() const -> usize;

private:
    std::vector<Entity> matched_;
};

} // namespace okn::ecs
