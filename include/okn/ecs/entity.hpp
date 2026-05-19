#pragma once

#include <okn/ecs/ecs_types.hpp>

namespace okn::ecs {

inline auto entity_index(Entity e) noexcept -> u32 { return e.index(); }
inline auto entity_generation(Entity e) noexcept -> u32 { return e.generation(); }
inline auto entity_is_valid(Entity e) noexcept -> bool { return e.is_valid(); }
inline auto make_entity(u32 index, u32 generation = 0) noexcept -> Entity { return Entity(index, generation); }

} // namespace okn::ecs
