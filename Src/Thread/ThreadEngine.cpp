#include "ThreadEngine.hpp"

ThreadEngine::ThreadEngine(size_t thread_count) {
    if (thread_count == 0) thread_count = 1;
    pool_.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i) {
        pool_.emplace_back([this] { WorkLoop(); });
    }
}

ThreadEngine::~ThreadEngine() {
    {
        std::scoped_lock lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto &t : pool_) {
        if (t.joinable()) t.join();
    }
}

void ThreadEngine::Submit(std::function<void()> task) {
    {
        std::scoped_lock lock(mutex_);
        if (stop_) throw std::runtime_error("ThreadEngine: submit on stopped pool");
        queue_.emplace(std::move(task));
    }
    cv_.notify_one();
}

void ThreadEngine::WorkLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
            if (stop_ && queue_.empty()) return;
            task = std::move(queue_.front());
            queue_.pop();
        }
        task();
    }
}
