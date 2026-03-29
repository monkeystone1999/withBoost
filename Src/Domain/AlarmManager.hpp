#pragma once
#include "../Thread/ThreadEngine.hpp"
#include <functional>
#include <string>
#include <deque>

/**
 * @file AlarmManager.hpp
 * @brief 알람 이벤트 발생 및 디스패칭 관리
 */

struct AlarmEvent {
    std::string title;
    std::string detail;
    int severity = 1;
};

class AlarmManager {
public:
    using Callback = std::function<void(AlarmEvent)>;

    explicit AlarmManager(ThreadEngine &pool) : pool_(pool) {}
    void dispatch(const std::string &json, Callback cb);

private:
    ThreadEngine &pool_;
};
