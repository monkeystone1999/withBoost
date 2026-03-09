#pragma once
#include "DeviceData.hpp"
#include <functional>
#include <mutex>
#include <string>
#include <vector>

// ============================================================
//  DeviceStore — Pure C++ device list (SUB_PI filter included)
//
//  WHEN:  Called from ThreadPool thread on every CAMERA(0x07) packet.
//  WHERE: Src/Domain
//  WHY:   DeviceModel::refreshFromJson previously filtered SUB_PI
//         entries on the GUI thread. The filter now runs on ThreadPool.
//  HOW:   updateFromJson() accepts the same CAMERA packet JSON as
//         CameraStore. It filters type=="SUB_PI" and calls cb with
//         the result. Core.cpp posts that result to DeviceModel.
// ============================================================

class DeviceStore {
public:
    using Callback = std::function<void(std::vector<DeviceData>)>;

    void updateFromJson(const std::string &json, Callback cb);

    std::vector<DeviceData> snapshot() const;

    // Thread-safe lookup by rtspUrl — used by DeviceModel Q_INVOKABLE helpers
    bool        hasDevice (const std::string &rtspUrl) const;
    bool        hasMotor  (const std::string &rtspUrl) const;
    std::string deviceIp  (const std::string &rtspUrl) const;

private:
    int findIndex(const std::string &rtspUrl) const;   // caller holds lock

    mutable std::mutex      m_mutex;
    std::vector<DeviceData> m_data;
};
