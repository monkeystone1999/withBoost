#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t size) : stop_(false) {
  for (size_t i = 0; i < size; ++i) {
    pool_.emplace_back([this] { worker_loop(); });
  }
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
  }
  cv_.notify_all();
  for (auto &ele : pool_) {
    ele.join();
  }
}

void ThreadPool::worker_loop() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
      if (stop_ && tasks_.empty())
        return;
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    task();
  }
}
