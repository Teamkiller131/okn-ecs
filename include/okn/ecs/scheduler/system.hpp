#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <vector>
#include <string>

namespace okn::ecs {

class World;

class System {
public:
    System() = default;
    virtual ~System() = default;

    virtual auto reads() const -> std::vector<ComponentTypeId> { return {}; }
    virtual auto writes() const -> std::vector<ComponentTypeId> { return {}; }

    virtual void execute(World& world, float delta_time) = 0;

    auto name() const -> const std::string& { return name_; }
    void set_name(const std::string& n) { name_ = n; }

    virtual auto before() const -> std::vector<std::string> { return {}; }
    virtual auto after() const -> std::vector<std::string> { return {}; }

private:
    std::string name_;
};

} // namespace okn::ecs
