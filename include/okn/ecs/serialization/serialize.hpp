#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <okn/ecs/reflection/registry.hpp>
#include <string>
#include <vector>
#include <cstring>

namespace okn::ecs {

class World;

class Serializer {
public:
    explicit Serializer(World& world);

    auto save() -> std::vector<u8>;
    auto load(const std::vector<u8>& data) -> bool;

    // Persist the World to / restore it from a file (the EKO1 byte stream written
    // verbatim). This is the "scene survives a restart" path: a scene saved here reloads
    // intact in a fresh World — including cross-entity references, which are remapped on
    // load (register their fields via entity_refs.hpp). Returns false on any file error.
    auto save_to_file(const std::string& path) -> bool;
    auto load_from_file(const std::string& path) -> bool;

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