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

/**
 * @file ThreadEngine.hpp
 * @brief 스레드 풀 엔진
 */

class ThreadEngine {
public:
    explicit ThreadEngine(size_t thread_count = std::thread::hardware_concurrency() / 2);
    ~ThreadEngine();

    ThreadEngine(const ThreadEngine&) = delete;
    ThreadEngine& operator=(const ThreadEngine&) = delete;
    ThreadEngine(ThreadEngine&&) = delete;
    ThreadEngine& operator=(ThreadEngine&&) = delete;

    void Submit(std::function<void()> task);

    template <typename T, typename... Arg>
    auto Submit(T &&t, Arg &&...args) -> std::future<std::invoke_result_t<T, Arg...>> {
        using result = std::invoke_result_t<T, Arg...>;
        auto promise = std::make_shared<std::promise<result>>();
        std::future<result> fut = promise->get_future();
        auto bound_args = std::make_tuple(std::forward<Arg>(args)...);
        
        std::function<void()> task = [func = std::forward<T>(t), args = std::move(bound_args), promise]() mutable {
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
            if (stop_) throw std::runtime_error("ThreadEngine: submit on stopped pool");
            queue_.emplace(std::move(task));
        }
        cv_.notify_one();
        return fut;
    }

private:
    void WorkLoop();

    std::condition_variable cv_;
    std::mutex mutex_;
    std::vector<std::thread> pool_;
    std::queue<std::function<void()>> queue_;
    bool stop_ = false;
};

/// 호환성을 위한 타입 에일리어스 (필요 시 제거 가능)
using ThreadPool = ThreadEngine;
