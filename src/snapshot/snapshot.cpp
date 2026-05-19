#include <okn/ecs/snapshot/snapshot.hpp>
#include <okn/ecs/serialization/serialize.hpp>

namespace okn::ecs {

Snapshot::Snapshot(World& world) : world_(&world) {}

auto Snapshot::capture() -> std::vector<u8> {
    Serializer s(*world_);
    return s.save();
}

auto Snapshot::restore(const std::vector<u8>& data) -> bool {
    Serializer s(*world_);
    return s.load(data);
}

Rollback::Rollback(World& world, u32 max_snapshots)
    : world_(&world), max_snapshots_(max_snapshots) {}

auto Rollback::save_frame(u32 frame) -> void {
    Snapshot s(*world_);
    auto data = s.capture();
    snapshots_.push_back({frame, std::move(data)});
    if (snapshots_.size() > max_snapshots_) snapshots_.erase(snapshots_.begin());
}

auto Rollback::restore_frame(u32 frame) -> bool {
    for (auto& fd : snapshots_) {
        if (fd.frame == frame) {
            Snapshot s(*world_);
            return s.restore(fd.data);
        }
    }
    return false;
}

void Rollback::clear() { snapshots_.clear(); }

} // namespace okn::ecs