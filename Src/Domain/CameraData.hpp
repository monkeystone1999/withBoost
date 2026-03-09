#pragma once
#include <string>

// ============================================================
//  CameraData.hpp — Plain C++ value type for one camera entry
//
//  WHY:  CameraEntry was defined inside CameraModel.hpp (a Qt header).
//        Pure C++ code in Src/ cannot include Qt headers.
//        This struct crosses the Qt / pure-C++ boundary freely.
//
//  RULE: No Qt types. No QObject. No includes from Qt/.
// ============================================================

struct CameraData {
    std::string title;
    std::string rtspUrl;
    std::string cameraType;   // "HANWHA" | "SUB_PI"
    bool        isOnline = false;
};
