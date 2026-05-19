#include <okn/ecs/query/query.hpp>
#include <okn/ecs/world.hpp>

namespace okn::ecs {

Query::Query(const World& world, const Filter& filter) {
    usize total = world.entity_count();
    matched_.reserve(total);

    // Collect component types for each alive entity
    // Iterate through all possible entity indices
    for (u32 idx = 0; matched_.size() < total && idx < world.next_entity_index_; ++idx) {
        if (world.entity_generations_.size() <= idx) break;
        u32 gen = world.entity_generations_[idx];
        if (gen == 0) continue;
        Entity entity(idx, gen);

        std::vector<ComponentTypeId> entity_types;
        for (const auto& [cid, store] : world.stores_) {
            if (store.has(entity)) {
                entity_types.push_back(cid);
            }
        }

        if (filter.matches(entity_types)) {
            matched_.push_back(entity);
        }
    }
}

auto Query::entities() const -> const std::vector<Entity>& {
    return matched_;
}

auto Query::entity_count() const -> usize {
    return matched_.size();
}

} // namespace okn::ecs
