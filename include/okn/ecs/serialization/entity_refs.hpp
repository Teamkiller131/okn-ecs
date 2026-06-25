#pragma once

// Entity-reference remapping for save/load. A component is serialized as opaque
// trivially-copyable bytes, so the deserializer can't tell which fields hold an
// Entity (a cross-entity reference). Register those fields here, by byte offset, and
// the loader will remap them from the SAVED entity ids to the freshly-assigned ones —
// otherwise a reloaded reference keeps a stale id and points at the wrong entity (or
// nothing). An offset for an unsaved/destroyed target remaps to the invalid entity.
//
// Keyed by World::component_type_id<T>() (the same id the serializer writes for the
// component's store), NOT the reflection ComponentRegistry id — those use different
// hashes and would not match.

#include <okn/ecs/world.hpp>

#include <initializer_list>
#include <unordered_map>
#include <vector>

namespace okn::ecs {

namespace detail {
// Process-wide map: component store id -> byte offsets of its Entity-ref fields.
inline auto entity_ref_registry() -> std::unordered_map<ComponentTypeId, std::vector<u32>>& {
    static std::unordered_map<ComponentTypeId, std::vector<u32>> reg;
    return reg;
}
}  // namespace detail

// Declare which fields of component T hold an Entity reference, e.g.
//   register_entity_ref_fields<Link>({ offsetof(Link, target) });
template <class T>
inline void register_entity_ref_fields(std::initializer_list<u32> byte_offsets) {
    detail::entity_ref_registry()[World::component_type_id<T>()] =
        std::vector<u32>(byte_offsets);
}

}  // namespace okn::ecs
