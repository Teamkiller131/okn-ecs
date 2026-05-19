#include <okn/ecs/events/event_bus.hpp>

namespace okn::ecs {

void EventBus::dispatch() {
    std::vector<std::function<void()>> pending_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_copy.swap(pending_);
    }
    for (auto& fn : pending_copy) {
        fn();
    }
}

} // namespace okn::ecs
