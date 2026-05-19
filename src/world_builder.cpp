#include <okn/ecs/world_builder.hpp>

namespace okn::ecs {

auto WorldBuilder::build() -> World {
    return std::move(world_);
}

} // namespace okn::ecs
