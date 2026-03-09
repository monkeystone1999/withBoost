#pragma once
#include <string>

// ============================================================
//  AlarmEvent — Plain C++ value type for one alarm occurrence
// ============================================================

struct AlarmEvent {
    std::string title;
    std::string detail;
    int         severity = 1;
};
