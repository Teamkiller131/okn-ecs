#pragma once

#include <okn/ecs/scheduler/job_adapter.hpp>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace okn::ecs {

// A concrete IJobSystem backed by a fixed pool of worker threads, so the
// Scheduler's parallel path actually runs systems concurrently. submit() queues
// a job for the workers; wait_all() blocks (on a condition variable) until every
// submitted job has run. The Scheduler fans out a level's systems with submit()
// then joins with wait_all() — no busy-spin.
class ThreadPoolJobSystem : public IJobSystem {
public:
    // thread_count == 0 uses hardware_concurrency() (at least 1).
    explicit ThreadPoolJobSystem(unsigned thread_count = 0);
    ~ThreadPoolJobSystem() override;

    ThreadPoolJobSystem(const ThreadPoolJobSystem&) = delete;
    auto operator=(const ThreadPoolJobSystem&) -> ThreadPoolJobSystem& = delete;

    void submit(std::function<void()> job) override;
    void wait_all() override;

    [[nodiscard]] auto worker_count() const -> unsigned { return static_cast<unsigned>(workers_.size()); }

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;
    mutable std::mutex mutex_;
    std::condition_variable job_available_;
    std::condition_variable all_done_;
    std::atomic<int> outstanding_{0};  // queued + running jobs not yet finished
    bool stop_ = false;
};

} // namespace okn::ecs
