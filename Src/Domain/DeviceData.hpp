#pragma once
#include <string>

// ============================================================
//  DeviceData.hpp — Plain C++ value type for one controllable device
//
//  WHY:  DeviceEntry was inside DeviceModel.hpp (Qt header).
//        The device filtering logic (SUB_PI only) needs to run
//        on the ThreadPool, not on the GUI thread.
// ============================================================

struct DeviceCapabilityData {
    bool motor  = true;
    bool ir     = false;
    bool heater = false;
};

struct DeviceData {
    std::string          rtspUrl;
    std::string          deviceIp;
    std::string          title;
    bool                 isOnline = false;
    DeviceCapabilityData cap;
};
