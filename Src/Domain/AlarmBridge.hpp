#pragma once
#include "AlarmManager.hpp"
#include <deque>

/**
 * @file AlarmBridge.hpp
 * @brief 알람 데이터와 UI/네트워크 간의 중계자
 */

class AlarmBridge {
public:
    void AddEvent(AlarmEvent ev) {
        events_.push_back(std::move(ev));
        if (events_.size() > 100) events_.pop_front();
    }

private:
    std::deque<AlarmEvent> events_;
};
