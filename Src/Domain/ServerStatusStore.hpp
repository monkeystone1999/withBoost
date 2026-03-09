#pragma once
#include "ServerStatusData.hpp"
#include <functional>
#include <mutex>
#include <string>

// ============================================================
//  ServerStatusStore — Pure C++ server + device status store
//
//  WHEN:  Called from ThreadPool on every DEVICE(0x04) packet (5s).
//  WHERE: Src/Domain
//  WHY:   ServerStatusModel::refreshFromJson previously parsed JSON
//         on the GUI thread. Admin DEVICE packet includes both
//         "devices" array and "server" object; user packet has only
//         "devices". The admin check lives here, not in QObject.
//  HOW:   updateFromJson() parses, writes m_status under lock, then
//         calls cb. Core.cpp posts cb result to ServerStatusModel.
// ============================================================

class ServerStatusStore {
public:
    using Callback = std::function<void(ServerStatusData)>;

    void updateFromJson(const std::string &json, Callback cb);

    ServerStatusData snapshot() const;

    // Thread-safe per-device queries for QML
    double deviceCpu   (const std::string &ip) const;
    double deviceMemory(const std::string &ip) const;
    double deviceTemp  (const std::string &ip) const;
    int    deviceUptime(const std::string &ip) const;

private:
    mutable std::mutex m_mutex;
    ServerStatusData   m_status;
};
