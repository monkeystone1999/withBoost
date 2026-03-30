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
 * @brief 고성능 범용 스레드 풀 엔진
 *
 * 이 파일은 시스템 전반에서 비동기 작업을 병렬로 처리하기 위한 스레드 풀의
 * 정의를 포함합니다. 가변 인자 탬플릿과 `std::future`를 통해 작업의 리턴값을
 * 비동기적으로 획득할 수 있습니다.
 */

/**
 * @class ThreadEngine
 * @brief 비동기 작업 큐 및 워커 스레드 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 생성 시 워커 스레드의 개수를 지정합니다. (지정하지 않을 경우 CPU 코어 수의
 * 절반으로 설정됩니다.)
 * 2. 단순 호출은 Submit(void_func)을 사용합니다.
 * 3. 리턴값이 필요한 작업은 Submit(func, args...)을 호출하여 리턴된 std::future
 * 객체로 결과를 대기합니다.
 */
class ThreadEngine {
public:
  /**
   * @brief 스레드 풀 생성자
   * @param thread_count 가동할 실제 스레드 개수
   */
  explicit ThreadEngine(
      size_t thread_count = std::thread::hardware_concurrency() / 2);

  /** @brief 소멸자: 실행 중인 작업을 모두 중지하고 스레드를 정상
   * 종료(Join)합니다. */
  ~ThreadEngine();

  /** @name Copy/Move 제한 (싱글톤 또는 명시적 소유권 준수) */
  ///@{
  ThreadEngine(const ThreadEngine &) = delete;
  ThreadEngine &operator=(const ThreadEngine &) = delete;
  ThreadEngine(ThreadEngine &&) = delete;
  ThreadEngine &operator=(ThreadEngine &&) = delete;
  ///@}

  /**
   * @brief 단순 작업 등록 (단방향 호출)
   * @param task 실행할 함수 객체 (리턴값 없음)
   */
  void Submit(std::function<void()> task);

  /** @brief 스레드 풀 명시적 종료 (작업 소진 후 정지) */
  void Shutdown();

  /**
   * @brief 비동기 결과 추적 작업 등록
   * @tparam T 함수 타입
   * @tparam Arg 인자 타입 패키
   * @param t 실행할 콜러블(Callable) 객체
   * @param args 함수에 전달할 인자들
   * @return std::future<...> 결과값을 획득하기 위한 퓨처 객체
   */
  template <typename T, typename... Arg>
  auto Submit(T &&t, Arg &&...args)
      -> std::future<std::invoke_result_t<T, Arg...>> {
    using result = std::invoke_result_t<T, Arg...>;
    auto promise = std::make_shared<std::promise<result>>();
    std::future<result> fut = promise->get_future();
    auto bound_args = std::make_tuple(std::forward<Arg>(args)...);

    std::function<void()> task = [func = std::forward<T>(t),
                                  args = std::move(bound_args),
                                  promise]() mutable {
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
        throw std::runtime_error("ThreadEngine: submit on stopped pool");
      queue_.emplace(std::move(task));
    }
    cv_.notify_one();
    return fut;
  }

private:
  /** @brief 워커 스레드가 상주하며 작업을 꺼내어 실행하는 루프 함수 */
  void WorkLoop();

  std::condition_variable cv_;              /**< 작업 대기/통지용 조건 변수 */
  std::mutex mutex_;                        /**< 작업 큐 접근 동기화용 뮤텍스 */
  std::vector<std::thread> pool_;           /**< 실제 운영 중인 스레드 집합 */
  std::queue<std::function<void()>> queue_; /**< 대기 중인 작업 우선순위 큐 */
  bool stop_ = false;                       /**< 풀 정지 상태 플래그 */
};

/**
 * @section Workflow Guide
 *
 * **[ThreadEngine 비동기 워크플로우]**
 *
 * 1. 작업 등록: `Submit`이 호출되면 내부 큐(`queue_`)에 함수를 삽입하고
 * `cv_.notify_one()`을 호출합니다.
 * 2. 작업 수임: 잠자고 있던 워커 스레드 중 하나가 깨어나 `unique_lock`을
 * 획득하고 큐에서 작업을 꺼냅니다.
 * 3. 실행 및 완료: 워커 스레드가 작업을 수행하며, 예외 발생 시 `promise`를 통해
 * 예외 정보를 호출측에 전달합니다.
 */
