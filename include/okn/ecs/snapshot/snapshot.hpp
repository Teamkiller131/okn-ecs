#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <vector>
#include <cstring>

namespace okn::ecs {

class World;

class Snapshot {
public:
    explicit Snapshot(World& world);
    auto capture() -> std::vector<u8>;
    auto restore(const std::vector<u8>& data) -> bool;

private:
    World* world_;
};

class Rollback {
public:
    explicit Rollback(World& world, u32 max_snapshots = 60);
    auto save_frame(u32 frame) -> void;
    auto restore_frame(u32 frame) -> bool;
    auto frame_count() const -> u32 { return static_cast<u32>(snapshots_.size()); }
    void clear();

private:
    World* world_;
    u32 max_snapshots_;
    struct FrameData { u32 frame; std::vector<u8> data; };
    std::vector<FrameData> snapshots_;
};

} // namespace okn::ecs