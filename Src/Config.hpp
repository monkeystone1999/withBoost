#pragma once
#include <array>
#include <string_view>

// ============================================================
//  Config.hpp — Compile-time deployment constants
//
//  WHY:  Login.cpp previously hard-coded "192.168.0.58:20000".
//        Any environment change required editing business logic.
//        All deployment knobs live here so only this file changes.
//
//  WHO uses it: Core.cpp, NetworkService.cpp
//  WHO must NOT use it: QObject adapters (they receive values via DI)
// ============================================================

namespace Config {

    // ── Network ──────────────────────────────────────────────
    constexpr std::string_view SERVER_HOST    = "192.168.0.58";
    constexpr std::string_view SERVER_PORT    = "20000";
    constexpr std::string_view SESSION_NAME   = "Main";

    // ── ThreadPool ───────────────────────────────────────────
    // 4 threads: camera-parse, device-parse, alarm-parse + 1 spare
    constexpr std::size_t THREAD_POOL_SIZE    = 4;

    // ── Video ────────────────────────────────────────────────
    //constexpr int  VIDEO_FPS_LIMIT_MS         = 41;   // ~24 fps
    constexpr int  VIDEO_BUFFER_POOL_SIZE_HD  = 8;
    constexpr int  VIDEO_BUFFER_POOL_SIZE_4K  = 3;
    constexpr int  SPLIT_AUTO_THRESHOLD_WIDTH = 2560;

    struct AutoSplitEntry {
        std::string_view rtspUrl;
        int              tileCount;
    };

    // URL -> split tile count. tileCount must be 1, 2, 3, or 4.
    // Fill this list per deployment to enable automatic split by camera URL.
    constexpr std::array<AutoSplitEntry, 0> AUTO_SPLIT_URL_TILE_MAP{};

    constexpr int autoSplitTileCountForUrl(std::string_view url) {
        for (const auto &entry : AUTO_SPLIT_URL_TILE_MAP) {
            if (entry.rtspUrl == url) {
                return entry.tileCount;
            }
        }
        return 1;
    }

} // namespace Config
