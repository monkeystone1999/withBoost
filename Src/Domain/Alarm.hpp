#pragma once

#include "../Thread/ThreadPool.hpp"
#include <functional>
#include <string>


// ============================================================
//  Alarm Domain
// ============================================================

struct AlarmEvent {
  std::string title;
  std::string detail;
  int severity = 1;
};

class AlarmDispatcher {
public:
  using Callback = std::function<void(AlarmEvent)>;

  explicit AlarmDispatcher(ThreadPool &pool) : pool_(pool) {}

  void dispatch(const std::string &json, Callback cb);

private:
  ThreadPool &pool_;
};

class AlarmBridge{

    std::deque<AlarmEvent> AlarmEvent_;
};
