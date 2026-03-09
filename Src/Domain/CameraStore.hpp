#pragma once
#include "CameraData.hpp"
#include <functional>
#include <mutex>
#include <string>
#include <vector>

// ============================================================
//  CameraStore — Pure C++ camera list with thread-safe snapshot
//
//  WHEN:  Called from ThreadPool thread on every CAMERA(0x07) packet.
//  WHERE: Src/Domain — no Qt dependency whatsoever.
//  WHY:   CameraModel::refreshFromJson previously parsed JSON on the
//         GUI thread every 5 seconds for 13 cameras.  Moving the parse
//         to the ThreadPool eliminates that 1-5 ms GUI stall.
//  HOW:   updateFromJson() parses, updates m_data under lock, then
//         fires the callback which Core.cpp posts back to the GUI thread.
//         The GUI thread then calls CameraModel::onStoreUpdated() with
//         an already-parsed snapshot — no JSON work on GUI thread.
// ============================================================

class CameraStore {
public:
    using Callback = std::function<void(std::vector<CameraData>)>;

    // Called from ThreadPool — parse JSON, update snapshot, invoke cb.
    // cb is always called, even if data is unchanged.
    void updateFromJson(const std::string &json, Callback cb);

    // Thread-safe read — GUI thread or anyone
    std::vector<CameraData> snapshot() const;

private:
    mutable std::mutex      m_mutex;
    std::vector<CameraData> m_data;
};
