#include <okn/ecs/serialization/serialize.hpp>
#include <okn/ecs/serialization/entity_refs.hpp>
#include <okn/ecs/world.hpp>

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace okn::ecs {

// Binary snapshot of a World's entities + their (trivially-copyable) component
// bytes. Format (host-endian; intended for same-build save-games, not a stable
// cross-version archive):
//
//   u32 magic ("EKO1"), u32 version
//   u32 entity_count
//   per entity:
//       u32 index, u32 generation        (the saved id; load remaps refs from it)
//       u32 component_count
//       per component (sorted by type id for reproducible output):
//           u64 type_id (the World store key), u32 size, <size> raw bytes
//
// Components are stored by their World store id (hash of typeid name) and copied
// verbatim — valid because every ECS component is trivially copyable (EcbComponent).

namespace {

constexpr u32 kMagic = 0x314F4B45u;  // 'E','K','O','1'
constexpr u32 kVersion = 1u;

template <class T>
void put(std::vector<u8>& buf, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>, "put() expects a POD");
    const auto* p = reinterpret_cast<const u8*>(&value);
    buf.insert(buf.end(), p, p + sizeof(T));
}

template <class T>
auto get(const u8*& ptr, const u8* end, T& out) -> bool {
    if (ptr + sizeof(T) > end) {
        return false;
    }
    std::memcpy(&out, ptr, sizeof(T));
    ptr += sizeof(T);
    return true;
}

} // namespace

Serializer::Serializer(World& world) : world_(&world) {}

auto Serializer::save() -> std::vector<u8> {
    std::vector<u8> buf;
    const World& w = *world_;

    put(buf, kMagic);
    put(buf, kVersion);

    // Live entities, in index order, excluding indices on the free list.
    const std::unordered_set<u32> free(w.free_entity_indices_.begin(),
                                       w.free_entity_indices_.end());
    std::vector<Entity> live;
    for (u32 idx = 0; idx < w.entity_generations_.size(); ++idx) {
        const u32 gen = w.entity_generations_[idx];
        if (gen != 0 && free.find(idx) == free.end()) {
            live.emplace_back(idx, gen);
        }
    }

    put(buf, static_cast<u32>(live.size()));
    for (const Entity e : live) {
        put(buf, e.index());
        put(buf, e.generation());

        // Components present on this entity, sorted by type id (reproducible).
        std::vector<std::pair<ComponentTypeId, const World::ComponentStore*>> comps;
        for (const auto& [cid, store] : w.stores_) {
            if (store.has(e)) {
                comps.emplace_back(cid, &store);
            }
        }
        std::sort(comps.begin(), comps.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        put(buf, static_cast<u32>(comps.size()));
        for (const auto& [cid, store] : comps) {
            put(buf, cid);
            put(buf, static_cast<u32>(store->component_size));
            const auto* bytes = static_cast<const u8*>(store->get(e));
            buf.insert(buf.end(), bytes, bytes + store->component_size);
        }
    }

    return buf;
}

auto Serializer::load(const std::vector<u8>& data) -> bool {
    Deserializer d(*world_);
    return d.deserialize(data);
}

Deserializer::Deserializer(World& world) : world_(&world) {}

auto Deserializer::deserialize(const std::vector<u8>& data) -> bool {
    const u8* ptr = data.data();
    const u8* const end = ptr + data.size();
    World& w = *world_;

    u32 magic = 0;
    u32 version = 0;
    if (!get(ptr, end, magic) || magic != kMagic) {
        return false;
    }
    if (!get(ptr, end, version) || version != kVersion) {
        return false;
    }

    u32 entity_count = 0;
    if (!get(ptr, end, entity_count)) {
        return false;
    }

    // Pass 1: create every entity up front (building saved-id -> new-id remap) and
    // record each component's byte range. Components are filled in only after the
    // whole remap is known, so cross-entity references can be patched correctly.
    std::unordered_map<u64, Entity> remap;
    struct PendingComp {
        Entity owner;
        ComponentTypeId cid;
        u32 size;
        const u8* bytes;
    };
    std::vector<PendingComp> pending;

    for (u32 i = 0; i < entity_count; ++i) {
        u32 saved_index = 0;
        u32 saved_gen = 0;
        if (!get(ptr, end, saved_index) || !get(ptr, end, saved_gen)) {
            return false;
        }
        const Entity e = w.create_entity();
        remap[Entity(saved_index, saved_gen).raw()] = e;

        u32 comp_count = 0;
        if (!get(ptr, end, comp_count)) {
            return false;
        }
        for (u32 j = 0; j < comp_count; ++j) {
            ComponentTypeId cid = 0;
            u32 size = 0;
            if (!get(ptr, end, cid) || !get(ptr, end, size)) {
                return false;
            }
            if (ptr + size > end) {
                return false;
            }
            pending.push_back({e, cid, size, ptr});
            ptr += size;
        }
    }

    // Pass 2: materialize each component, remapping any registered Entity-ref field
    // from its saved id to the freshly-assigned one (unknown targets -> invalid).
    const auto& ref_reg = detail::entity_ref_registry();
    for (const auto& pc : pending) {
        auto it = w.stores_.find(pc.cid);
        if (it == w.stores_.end()) {
            auto [inserted, _] = w.stores_.emplace(pc.cid, World::ComponentStore{});
            inserted->second.component_size = pc.size;
            it = inserted;
        }
        World::ComponentStore& store = it->second;
        if (store.component_size != pc.size) {
            return false;  // type id collision / size mismatch -> corrupt input
        }

        std::vector<u8> bytes(pc.bytes, pc.bytes + pc.size);
        const auto rit = ref_reg.find(pc.cid);
        if (rit != ref_reg.end()) {
            for (const u32 off : rit->second) {
                if (off + sizeof(u64) > pc.size) {
                    continue;  // a stale/incorrect offset can't corrupt the blob
                }
                u64 saved_raw = 0;
                std::memcpy(&saved_raw, bytes.data() + off, sizeof(u64));
                const auto mit = remap.find(saved_raw);
                const u64 new_raw = (mit != remap.end()) ? mit->second.raw() : Entity{}.raw();
                std::memcpy(bytes.data() + off, &new_raw, sizeof(u64));
            }
        }

        w.ensure_sparse_capacity(store, pc.owner.index());
        if (store.has(pc.owner)) {
            std::memcpy(store.get(pc.owner), bytes.data(), pc.size);
        } else {
            store.push(pc.owner, bytes.data());
        }
    }

    return true;
}

} // namespace okn::ecs
