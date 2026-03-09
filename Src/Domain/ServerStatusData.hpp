#pragma once
#include <string>
#include <vector>

// ============================================================
//  ServerStatusData.hpp — Plain C++ value types for server/device stats
//
//  WHY:  The DEVICE(0x04) packet carries cpu/mem/temp for admin.
//        Parsing on the GUI thread blocks rendering every 5 seconds.
//        These structs cross the ThreadPool → GUI boundary safely.
// ============================================================

struct DeviceStatusData {
    std::string ip;
    double      cpu           = 0.0;
    double      memory        = 0.0;
    double      temp          = 0.0;
    int         uptime        = 0;
    int         pendingEvents = 0;
};

struct ServerStatusData {
    double cpu       = 0.0;
    double memory    = 0.0;
    double temp      = 0.0;
    int    uptime    = 0;
    bool   available = false;   // true only when "server" key present (admin)

    std::vector<DeviceStatusData> devices;
};
