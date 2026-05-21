#pragma once
#include <okn/math/algebra/vec3.hpp>
#include <okn/core/api/types.hpp>

namespace okn::ecs {

using okn::core::u32;
using okn::core::f32;
using okn::math::Vec3;

// Attach to any entity to give it physics properties
struct RigidBodyComponent {
    Vec3 velocity{};
    Vec3 angular_velocity{};
    f32 mass = 1.0f;
    f32 restitution = 0.5f;
    f32 friction = 0.5f;
    bool is_static = false;
    bool ccd_enabled = false;
    u32 physics_body_id = 0; // set by PhysicsSystem
};

// Attach to define the entity's collision shape
enum class ColliderShape : u32 { kSphere = 0, kBox = 1, kCapsule = 2 };

struct ColliderComponent {
    ColliderShape shape = ColliderShape::kSphere;
    f32 radius = 0.5f;       // for sphere
    Vec3 half_extents{0.5f, 0.5f, 0.5f}; // for box
    f32 capsule_radius = 0.35f;
    f32 capsule_half_height = 0.5f;
};

} // namespace okn::ecs
