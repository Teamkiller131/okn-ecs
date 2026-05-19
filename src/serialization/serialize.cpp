#include <okn/ecs/serialization/serialize.hpp>
#include <okn/ecs/world.hpp>
#include <okn/ecs/reflection/registry.hpp>
#include <cstring>

namespace okn::ecs {

Serializer::Serializer(World& world) : world_(&world) {}

auto Serializer::save() -> std::vector<u8> {
    std::vector<u8> buf;
    // TODO: full integration later; framework is ready
    return buf;
}

auto Serializer::load(const std::vector<u8>& data) -> bool {
    Deserializer d(*world_);
    return d.deserialize(data);
}

Deserializer::Deserializer(World& world) : world_(&world) {}

auto Deserializer::deserialize(const std::vector<u8>& data) -> bool {
    auto* ptr = data.data();
    auto end = data.data() + data.size();
    if (ptr + 4 > end) return false;
    u32 ec; std::memcpy(&ec, ptr, 4); ptr += 4;
    auto& reg = ComponentRegistry::instance();
    for (u32 i = 0; i < ec; ++i) {
        if (ptr + 12 > end) return false;
        u64 raw_id; u32 gen;
        std::memcpy(&raw_id, ptr, 8); ptr += 8;
        std::memcpy(&gen, ptr, 4); ptr += 4;
        Entity e = world_->create_entity();
        u32 cc;
        if (ptr + 4 > end) return false;
        std::memcpy(&cc, ptr, 4); ptr += 4;
        for (u32 j = 0; j < cc; ++j) {
            if (ptr + 16 > end) return false;
            ComponentTypeId tid; u32 sz;
            std::memcpy(&tid, ptr, 8); ptr += 8;
            std::memcpy(&sz, ptr, 4); ptr += 4;
            if (ptr + sz > end) return false;
            auto* info = reg.get_info(tid);
            if (info && info->copy_constructor) {
                u8 tmp[256];
                if (sz <= 256) { std::memcpy(tmp, ptr, sz); }
            }
            ptr += sz;
        }
    }
    return true;
}

} // namespace okn::ecs