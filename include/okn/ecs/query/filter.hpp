#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <vector>
#include <algorithm>

namespace okn::ecs {

struct Filter {
    std::vector<ComponentTypeId> with;
    std::vector<ComponentTypeId> without;
    std::vector<ComponentTypeId> optional;

    auto matches(const std::vector<ComponentTypeId>& component_types) const -> bool {
        for (auto cid : with) {
            if (std::find(component_types.begin(), component_types.end(), cid) == component_types.end()) {
                return false;
            }
        }
        for (auto cid : without) {
            if (std::find(component_types.begin(), component_types.end(), cid) != component_types.end()) {
                return false;
            }
        }
        return true;
    }
};

} // namespace okn::ecs
