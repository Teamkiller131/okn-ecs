#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <functional>

namespace okn::ecs {

class IJobSystem {
public:
    virtual ~IJobSystem() = default;

    virtual void submit(std::function<void()> job) = 0;
    virtual void wait_all() = 0;
};

} // namespace okn::ecs
