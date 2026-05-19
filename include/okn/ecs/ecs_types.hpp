#pragma once

#include <okn/core/api/types.hpp>
#include <okn/core/api/id.hpp>

namespace okn::ecs {

using okn::core::u8;
using okn::core::u32;
using okn::core::u64;
using okn::core::usize;

using Entity = okn::core::Id;

using ComponentTypeId = u64;

inline constexpr Entity kInvalidEntity{};

} // namespace okn::ecs
