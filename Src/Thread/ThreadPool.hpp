#pragma once
#include <concepts>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

class ThreadPool {
public:
  explicit ThreadPool(
      std::size_t thread_count = std::thread::hardware_concurrency() / 2)
      : stop_(false) {
    if (thread_count == 0)
      thread_count = 1;

    pool_.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) {
      pool_.emplace_back([this] { WorkLoop(); });
    }
  }
  ~ThreadPool() {
    {
      std::scoped_lock lock(mutex_);
      stop_ = true;
    }
    cv_.notify_all();
    for (auto &t : pool_)
      if (t.joinable())
        t.join();
  }
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;
  void WorkLoop();
  void Submit(std::function<void()>);
  template <typename T, typename... Arg>
  auto Submit(T &&t, Arg &&...args)
      -> std::future<std::invoke_result_t<T, Arg...>> {
    using result = std::invoke_result_t<T, Arg...>;
    auto promise =
        std::make_shared<std::promise<result>>(); // promise 를 shared_ptr 로 힙
                                                  // 할당 → 람다 캡처 가능
    std::future<result> fut = promise->get_future();
    auto bound_args = std::make_tuple(
        std::forward<Arg>(args)...); // 인자를 tuple 로 묶어 람다 안에 이동 캡처
    std::function<void()> task =
        [func = std::forward<T>(t), args = std::move(bound_args),
         promise]() mutable // void 반환과 non-void 반환을 if constexpr 로 분기
    {
      try {
        if constexpr (std::is_void_v<result>) {
          std::apply(func, std::move(args));
          promise->set_value();
        } else {
          promise->set_value(std::apply(func, std::move(args)));
        }
      } catch (...) {
        promise->set_exception(std::current_exception());
      }
    };
    {
      std::scoped_lock lock(mutex_);
      if (stop_)
        throw std::runtime_error("ThreadPool: submit on stopped pool");
      queue_.emplace(std::move(task));
    }
    cv_.notify_one();
    return fut;
  };

private:
  std::condition_variable cv_;
  std::mutex mutex_;
  std::vector<std::thread> pool_;
  std::queue<std::function<void()>> queue_;
  bool stop_;
};
