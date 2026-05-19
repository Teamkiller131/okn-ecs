#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <vector>
#include <mutex>

namespace okn::ecs {

template <class T>
class EventQueue {
public:
    void push(const T& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(event);
    }

    auto pop(T& out) -> bool {
        std::lock_guard<std::mutex> lock(mutex_);
        if (events_.empty()) {
            return false;
        }
        out = events_.back();
        events_.pop_back();
        return true;
    }

    auto empty() const -> bool {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.clear();
    }

    auto size() const -> usize {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.size();
    }

private:
    std::vector<T> events_;
    mutable std::mutex mutex_;
};

} // namespace okn::ecs
