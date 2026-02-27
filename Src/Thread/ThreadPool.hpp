#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t size);

    ~ThreadPool();

  template <typename F, typename... Args>
  auto submit(F &&f, Args &&...args)
      -> std::future<std::invoke_result_t<F, Args...>> {
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<ReturnType> future = task->get_future();

    {
      std::unique_lock lock(mutex_);

      if (stop_)
        throw std::runtime_error("ThreadPool stopped");

      tasks_.emplace([task]() { (*task)(); });
    }

    cv_.notify_one();
    return future;
  }

private:
  void worker_loop();

private:
  std::vector<std::thread> pool_;
  std::queue<std::function<void()>> tasks_;

  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> stop_;
};
