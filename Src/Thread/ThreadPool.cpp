#include "ThreadPool.hpp"

void ThreadPool::Submit(std::function<void()> task) {
  {
    std::scoped_lock<std::mutex> lock(mutex_);
    queue_.emplace(std::move(task));
  }
  cv_.notify_one();
}

void ThreadPool::WorkLoop() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock lock(mutex_);
      // stop_ 이고 큐가 비어 있으면 종료
      // 큐에 남은 태스크가 있으면 drain 후 종료
      cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });

      if (stop_ && queue_.empty())
        return;

      task = std::move(queue_.front());
      queue_.pop();
    }
    task();
  }
}
