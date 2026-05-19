#pragma once

#include <okn/ecs/world.hpp>

namespace okn::ecs {

class WorldBuilder {
public:
    WorldBuilder() = default;

    template <class T>
    auto register_component() -> WorldBuilder&;

    auto build() -> World;

private:
    World world_;
};

template <class T>
inline auto WorldBuilder::register_component() -> WorldBuilder& {
    world_.ensure_store<T>();
    return *this;
}

} // namespace okn::ecs
