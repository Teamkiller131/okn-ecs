#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <cstring>
#include <utility>

namespace okn::ecs {

template <class T>
class SparseSet {
public:
    SparseSet() = default;

    ~SparseSet() {
        delete[] dense_;
        delete[] entities_;
        delete[] sparse_;
    }

    SparseSet(const SparseSet&) = delete;
    auto operator=(const SparseSet&) -> SparseSet& = delete;

    SparseSet(SparseSet&& other) noexcept
        : dense_(other.dense_)
        , entities_(other.entities_)
        , sparse_(other.sparse_)
        , count_(other.count_)
        , capacity_(other.capacity_)
        , sparse_capacity_(other.sparse_capacity_) {
        other.dense_ = nullptr;
        other.entities_ = nullptr;
        other.sparse_ = nullptr;
        other.count_ = 0;
        other.capacity_ = 0;
        other.sparse_capacity_ = 0;
    }

    auto operator=(SparseSet&& other) noexcept -> SparseSet& {
        if (this != &other) {
            delete[] dense_;
            delete[] entities_;
            delete[] sparse_;
            dense_ = other.dense_;
            entities_ = other.entities_;
            sparse_ = other.sparse_;
            count_ = other.count_;
            capacity_ = other.capacity_;
            sparse_capacity_ = other.sparse_capacity_;
            other.dense_ = nullptr;
            other.entities_ = nullptr;
            other.sparse_ = nullptr;
            other.count_ = 0;
            other.capacity_ = 0;
            other.sparse_capacity_ = 0;
        }
        return *this;
    }

    void insert(Entity entity, const T& value) {
        auto idx = static_cast<usize>(entity.index());
        if (idx >= sparse_capacity_) {
            grow_sparse(idx + 1);
        }
        if (sparse_[idx] != kInvalidIndex) {
            dense_[sparse_[idx]] = value;
            return;
        }
        if (count_ >= capacity_) {
            grow(capacity_ == 0 ? 64 : capacity_ * 2);
        }
        usize slot = count_;
        dense_[slot] = value;
        entities_[slot] = entity;
        sparse_[idx] = slot;
        ++count_;
    }

    void remove(Entity entity) {
        auto idx = static_cast<usize>(entity.index());
        if (idx >= sparse_capacity_ || sparse_[idx] == kInvalidIndex) {
            return;
        }
        usize slot = sparse_[idx];
        usize last = count_ - 1;
        if (slot != last) {
            dense_[slot] = std::move(dense_[last]);
            entities_[slot] = entities_[last];
            sparse_[static_cast<usize>(entities_[last].index())] = slot;
        }
        sparse_[idx] = kInvalidIndex;
        --count_;
    }

    auto get(Entity entity) -> T* {
        auto idx = static_cast<usize>(entity.index());
        if (idx < sparse_capacity_ && sparse_[idx] != kInvalidIndex) {
            return &dense_[sparse_[idx]];
        }
        return nullptr;
    }

    auto get(Entity entity) const -> const T* {
        auto idx = static_cast<usize>(entity.index());
        if (idx < sparse_capacity_ && sparse_[idx] != kInvalidIndex) {
            return &dense_[sparse_[idx]];
        }
        return nullptr;
    }

    auto contains(Entity entity) const -> bool {
        auto idx = static_cast<usize>(entity.index());
        return idx < sparse_capacity_ && sparse_[idx] != kInvalidIndex;
    }

    auto size() const -> usize { return count_; }

    void clear() {
        count_ = 0;
        if (sparse_) {
            std::memset(sparse_, 0xFF, sparse_capacity_ * sizeof(usize));
        }
    }

    auto dense() -> T* { return dense_; }
    auto dense() const -> const T* { return dense_; }
    auto entities() const -> const Entity* { return entities_; }
    auto dense_size() const -> usize { return count_; }

private:
    static constexpr usize kInvalidIndex = static_cast<usize>(-1);

    T* dense_ = nullptr;
    Entity* entities_ = nullptr;
    usize* sparse_ = nullptr;
    usize count_ = 0;
    usize capacity_ = 0;
    usize sparse_capacity_ = 0;

    void grow(usize new_cap) {
        auto* new_dense = new T[new_cap];
        auto* new_entities = new Entity[new_cap];
        if (dense_) {
            for (usize i = 0; i < count_; ++i) {
                new_dense[i] = std::move(dense_[i]);
            }
            std::memcpy(new_entities, entities_, count_ * sizeof(Entity));
            delete[] dense_;
            delete[] entities_;
        }
        dense_ = new_dense;
        entities_ = new_entities;
        capacity_ = new_cap;
    }

    void grow_sparse(usize required_index) {
        usize new_cap = sparse_capacity_ == 0 ? 1024 : sparse_capacity_ * 2;
        while (new_cap <= required_index) {
            new_cap *= 2;
        }
        auto* new_sparse = new usize[new_cap];
        std::memset(new_sparse, 0xFF, new_cap * sizeof(usize));
        if (sparse_) {
            std::memcpy(new_sparse, sparse_, sparse_capacity_ * sizeof(usize));
            delete[] sparse_;
        }
        sparse_ = new_sparse;
        sparse_capacity_ = new_cap;
    }
};

} // namespace okn::ecs
