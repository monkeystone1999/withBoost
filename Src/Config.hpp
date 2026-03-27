#pragma once
#include <array>
#include <sigslot/signal.hpp>
#include <string_view>
#include <yaml-cpp/yaml.h>
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

class ReadSettings{
public:
    ReadSettings(){
        YAML::Node Settings = YAML::LoadFile("Settings.yaml");
        ServerIP = Settings["ServerIP"].as<std::string>();
        ServerPort = Settings["ServerPort"].as<std::string>();
    }
    std::string getServerIP() const {
        return ServerIP;
    }
    std::string getServerPort() const {
        return ServerPort;
    }
private:
    std::string ServerIP;
    std::string ServerPort;
};

class ExternSettings{
public:
    static ExternSettings& getInstance(){
        static ExternSettings set;
        return set;
    }
    ReadSettings ReadSettings_;
};




// ── Network ──────────────────────────────────────────────
constexpr std::string_view SERVER_HOST = "192.168.0.58";
constexpr std::string_view SERVER_PORT = "20000";
constexpr std::string_view SESSION_NAME = "Main";

// ── ThreadPool ───────────────────────────────────────────
// Removed THREAD_POOL_SIZE to dynamically use hardware_concurrency() / 2

// ── Video ────────────────────────────────────────────────
// constexpr int  VIDEO_FPS_LIMIT_MS         = 41;   // ~24 fps
constexpr int VIDEO_BUFFER_POOL_SIZE_HD = 8;
constexpr int VIDEO_BUFFER_POOL_SIZE_4K = 3;
constexpr int SPLIT_AUTO_THRESHOLD_WIDTH = 2560;

struct AutoSplitEntry {
  std::string_view cameraId;
  int tileCount;
};

// CameraID -> split tile count. tileCount must be 1, 2, 3, or 4.
// Fill this list per deployment to enable automatic split by camera id.
constexpr std::array<AutoSplitEntry, 0> AUTO_SPLIT_CAMERA_ID_TILE_MAP{};

constexpr int autoSplitTileCountForCameraId(std::string_view cameraId) {
  for (const auto &entry : AUTO_SPLIT_CAMERA_ID_TILE_MAP) {
    if (entry.cameraId == cameraId) {
      return entry.tileCount;
    }
  }
  return 1;
}

template <typename T> class Observable {
public:
  sigslot::signal<T> on_changed;
  Observable &operator=(const T &t) {
    if (t_ != t) {
      t_ = t;
      on_changed(t);
    }
    return *this;
  }
  const T &get() const { return t_; }
  operator const T &() const { return t_; }

private:
  T t_{};
};

class AppState {
public:
  static AppState &getInstance() {
    static AppState app;
    return app;
  }
  enum class User : uint8_t { LogOut, Admin, Normal };
  User User_{User::LogOut};
};
} // namespace Config
