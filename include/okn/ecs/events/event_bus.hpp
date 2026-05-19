#pragma once

#include <okn/ecs/ecs_types.hpp>
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace okn::ecs {

class EventBus {
public:
    template <class T>
    void subscribe(std::function<void(const T&)> handler) {
        u64 tid = event_type_id<T>();
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[tid].push_back([h = std::move(handler)](const void* data) {
            h(*static_cast<const T*>(data));
        });
    }

    template <class T>
    void publish(const T& event) {
        u64 tid = event_type_id<T>();
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back([this, tid, event]() {
            auto it = handlers_.find(tid);
            if (it != handlers_.end()) {
                for (auto& handler : it->second) {
                    handler(&event);
                }
            }
        });
    }

    void dispatch();

    auto handler_count() const -> usize {
        std::lock_guard<std::mutex> lock(mutex_);
        return handlers_.size();
    }

    auto pending_count() const -> usize {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_.size();
    }

private:
    std::unordered_map<u64, std::vector<std::function<void(const void*)>>> handlers_;
    std::vector<std::function<void()>> pending_;
    mutable std::mutex mutex_;

    template <class T>
    static auto event_type_id() -> u64 {
        static const char marker = 0;
        return reinterpret_cast<u64>(&marker);
    }
};

} // namespace okn::ecs
