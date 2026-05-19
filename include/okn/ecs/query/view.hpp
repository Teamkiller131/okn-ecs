#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/entity.hpp>
#include <tuple>
#include <vector>

namespace okn::ecs {

class World;

template <class... Components>
class View {
public:
    explicit View(const World& world, std::vector<Entity> entities)
        : entities_(std::move(entities)), world_(&world) {}

    class Iterator {
    public:
        Iterator(const std::vector<Entity>& entities, usize index, const World* world)
            : entities_(&entities), index_(index), world_(world) {}

        auto operator!=(const Iterator& other) const -> bool {
            return index_ != other.index_;
        }

        auto operator*() const -> std::tuple<Entity, Components*...> {
            Entity entity = (*entities_)[index_];
            return std::make_tuple(entity, get_component_ptr<Components>(entity)...);
        }

        auto operator++() -> Iterator& {
            ++index_;
            return *this;
        }

    private:
        template <class T>
        auto get_component_ptr(Entity entity) const -> T* {
            return const_cast<World*>(world_)->get_component<T>(entity);
        }

        const std::vector<Entity>* entities_;
        usize index_;
        const World* world_;
    };

    auto begin() -> Iterator {
        return Iterator(entities_, 0, world_);
    }

    auto end() -> Iterator {
        return Iterator(entities_, entities_.size(), world_);
    }

    auto size() const -> usize {
        return entities_.size();
    }

    auto empty() const -> bool {
        return entities_.empty();
    }

private:
    std::vector<Entity> entities_;
    const World* world_;
};

} // namespace okn::ecs
