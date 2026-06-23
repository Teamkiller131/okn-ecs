#include <okn/ecs/scheduler/thread_pool_job_system.hpp>

#include <utility>

namespace okn::ecs {

ThreadPoolJobSystem::ThreadPoolJobSystem(unsigned thread_count) {
    if (thread_count == 0) {
        thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) { thread_count = 1; }
    }
    workers_.reserve(thread_count);
    for (unsigned i = 0; i < thread_count; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

ThreadPoolJobSystem::~ThreadPoolJobSystem() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    job_available_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) { worker.join(); }
    }
}

void ThreadPoolJobSystem::submit(std::function<void()> job) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_.push(std::move(job));
        outstanding_.fetch_add(1, std::memory_order_relaxed);
    }
    job_available_.notify_one();
}

void ThreadPoolJobSystem::wait_all() {
    std::unique_lock<std::mutex> lock(mutex_);
    all_done_.wait(lock, [this] { return outstanding_.load(std::memory_order_acquire) == 0; });
}

void ThreadPoolJobSystem::worker_loop() {
    for (;;) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            job_available_.wait(lock, [this] { return stop_ || !jobs_.empty(); });
            if (stop_ && jobs_.empty()) { return; }
            job = std::move(jobs_.front());
            jobs_.pop();
        }

        job();

        if (outstanding_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // Last outstanding job finished — wake any wait_all() waiters.
            std::lock_guard<std::mutex> lock(mutex_);
            all_done_.notify_all();
        }
    }
}

} // namespace okn::ecs
