#pragma once
#include "AlarmEvent.hpp"
#include "../Thread/ThreadPool.hpp"
#include <functional>
#include <string>

// ============================================================
//  AlarmDispatcher — ThreadPool-based AI packet parser
//
//  WHEN:  Called from AlarmManager::onAiJson slot (GUI thread).
//  WHERE: Src/Domain
//  WHY:   AlarmManager::parseAiMessage previously parsed AI JSON
//         on the GUI thread. AI packets can be complex and frequent.
//         The ThreadPool handles the parse; the result callback
//         is posted back to the GUI thread by Core.cpp / AlarmManager.
//  HOW:   dispatch() submits a task to ThreadPool. The task parses
//         JSON and calls cb with an AlarmEvent if the packet type
//         is "alarm". cb must be thread-safe (typically an
//         invokeMethod lambda posted to GUI thread).
// ============================================================

class AlarmDispatcher {
public:
    using Callback = std::function<void(AlarmEvent)>;

    explicit AlarmDispatcher(ThreadPool &pool) : m_pool(pool) {}

    // Thread-safe: may be called from any thread.
    // cb is invoked on the ThreadPool thread — caller must post to GUI if needed.
    void dispatch(const std::string &json, Callback cb);

private:
    ThreadPool &m_pool;   // non-owning reference — owned by Core
};
