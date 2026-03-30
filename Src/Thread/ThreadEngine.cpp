#include "ThreadEngine.hpp"

ThreadEngine::ThreadEngine(size_t thread_count) {
  if (thread_count == 0)
    thread_count = 1;
  pool_.reserve(thread_count);
  for (size_t i = 0; i < thread_count; ++i) {
    pool_.emplace_back([this] { WorkLoop(); });
  }
}

ThreadEngine::~ThreadEngine() { Shutdown(); }

void ThreadEngine::Shutdown() {
  {
    std::unique_lock lock(mutex_);
    if (stop_)
      return;
    stop_ = true;
  }
  cv_.notify_all();
  for (auto &t : pool_) {
    if (t.joinable())
      t.join();
  }
}

void ThreadEngine::Submit(std::function<void()> task) {
  {
    std::scoped_lock lock(mutex_);
    if (stop_)
      throw std::runtime_error("ThreadEngine: submit on stopped pool");
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
      if (stop_ && queue_.empty())
        return;
      task = std::move(queue_.front());
      queue_.pop();
    }
    task();
  }
}

/**
 * @section Workflow Guide
 *
 * **[ThreadEngine 구현 가이드]**
 *
 * 1. 무한 루프 제어 (`WorkLoop`):
 *    - `cv_.wait`을 통해 CPU 점유를 최소화하며, `stop_` 플래그와 큐의 공백
 * 여부를 동시에 체크합니다.
 *    - 풀 종료 시에도 큐에 남아있는 작업은 모두 소진한 후 스레드가 종료되도록
 * 설계되었습니다.
 *
 * 2. 소멸 절차:
 *    - `stop_ = true` 설정 후 `notify_all`을 통해 모든 워커를 깨워 종료를
 * 유도합니다.
 *    - `join()`을 순차적으로 호출하여 모든 스레드 자원이 안전하게 회수됨을
 * 보장합니다.
 *
 * 3. 예외 안전성:
 *    - `Submit` 내부에서 `scoped_lock`을 사용하여 큐 조작 중의 레이스 컨디션을
 * 방지합니다.
 */
