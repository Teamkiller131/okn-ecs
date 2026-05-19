#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/reflection/registry.hpp>
#include <vector>
#include <cstring>

namespace okn::ecs {

class World;

class Serializer {
public:
    explicit Serializer(World& world);

    auto save() -> std::vector<u8>;
    auto load(const std::vector<u8>& data) -> bool;

private:
    World* world_;
};

class Deserializer {
public:
    explicit Deserializer(World& world);
    auto deserialize(const std::vector<u8>& data) -> bool;
private:
    World* world_;
};

} // namespace okn::ecs