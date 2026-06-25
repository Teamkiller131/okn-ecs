#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/entity.hpp>

#include <array>
#include <tuple>
#include <utility>
#include <vector>

namespace okn::ecs {

class World;

// A query result: the matched entities plus, cached once at query() time, a direct
// pointer to each component's store. Iterating dereferences each component through its
// cached store (one sparse-set lookup) — replacing the old path's per-entity, per-
// component hash-map store resolution.
//
// Two invariants make this safe to hold across the iteration: (1) the matched set is an
// eager snapshot taken in query(), so later spawns/despawns don't shift it; (2) the
// cached ComponentStore* stay valid because a store is NEVER erased or reassigned in
// stores_ — and std::unordered_map only invalidates pointers/references to a mapped
// value on erase of that element (a rehash from inserting a new component type does
// not). A destroyed entity in the snapshot yields a null component pointer, exactly as
// the old per-entity get_component() path did.
//
// This header is included only by world.hpp, after World is fully defined, so it may
// name the (private, friend-granted) World::ComponentStore type.
template <class... Components>
class View {
    static constexpr usize kN = sizeof...(Components);
    using Stores = std::array<World::ComponentStore*, kN>;

public:
    View(std::vector<Entity> entities, Stores stores)
        : entities_(std::move(entities)), stores_(stores) {}

    class Iterator {
    public:
        Iterator(const std::vector<Entity>& entities, usize index, const Stores& stores)
            : entities_(&entities), index_(index), stores_(&stores) {}

        auto operator!=(const Iterator& other) const -> bool { return index_ != other.index_; }
        auto operator++() -> Iterator& { ++index_; return *this; }

        auto operator*() const -> std::tuple<Entity, Components*...> {
            return deref(std::make_index_sequence<kN>{});
        }

    private:
        template <usize... Is>
        auto deref(std::index_sequence<Is...>) const -> std::tuple<Entity, Components*...> {
            const Entity entity = (*entities_)[index_];
            return std::make_tuple(entity,
                                   static_cast<Components*>((*stores_)[Is]->get(entity))...);
        }

        const std::vector<Entity>* entities_;
        usize index_;
        const Stores* stores_;
    };

    auto begin() -> Iterator { return Iterator(entities_, 0, stores_); }
    auto end() -> Iterator { return Iterator(entities_, entities_.size(), stores_); }
    auto size() const -> usize { return entities_.size(); }
    auto empty() const -> bool { return entities_.empty(); }

private:
    std::vector<Entity> entities_;
    Stores stores_{};
};

} // namespace okn::ecs
