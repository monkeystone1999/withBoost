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

/**
 * @file Config.hpp
 * @brief 프로젝트 전역 구성 및 상수 정의
 *
 * 이 파일은 시스템 운영에 필요한 배포 상수, 네트워크 설정,
 * 그리고 애플리케이션의 전역 상태를 관리하는 클래스들을 포함합니다.
 * 비즈니스 로직에서의 하드코딩을 방지하고 설정 변경 시 이 파일만 수정하도록
 * 설계되었습니다.
 */

namespace Config {

/**
 * @class ReadSettings
 * @brief 외부 설정 파일(YAML) 로더
 *
 * **Standard Usage Methodology:**
 * 1. 생성 시 "Settings.yaml" 파일을 찾아 서버 IP와 포트 정보를 메모리로
 * 로드합니다.
 * 2. getServerIP(), getServerPort()를 통해 로드된 정보를 참조합니다.
 */
class ReadSettings {
public:
  ReadSettings() {
    YAML::Node Settings = YAML::LoadFile("Settings.yaml");
    ServerIP = Settings["ServerIP"].as<std::string>();
    ServerPort = Settings["ServerPort"].as<std::string>();
  }
  /** @return std::string 로드된 서버 IP 주소 */
  std::string getServerIP() const { return ServerIP; }
  /** @return std::string 로드된 서버 포트 번호 */
  std::string getServerPort() const { return ServerPort; }

private:
  std::string ServerIP;
  std::string ServerPort;
};

/**
 * @class ExternSettings
 * @brief 외부 설정 접근을 위한 싱글톤 래퍼
 */
class ExternSettings {
public:
  static ExternSettings &getInstance() {
    static ExternSettings set;
    return set;
  }
  ReadSettings ReadSettings_; /**< 실질적인 설정 데이터 로더 */
};

// ── Network ──────────────────────────────────────────────
/** @brief 네트워크 세션 식별자 명칭 */
constexpr std::string_view SESSION_NAME = "Main";

// ── Video ────────────────────────────────────────────────
/** @brief HD급 영상을 위한 비디오 버퍼 풀 크기 */
constexpr int VIDEO_BUFFER_POOL_SIZE_HD = 8;
/** @brief 4K급 영상을 위한 비디오 버퍼 풀 크기 */
constexpr int VIDEO_BUFFER_POOL_SIZE_4K = 3;
/** @brief 자동 화면 분할을 트리거하는 가로 해상도 임계값 */
constexpr int SPLIT_AUTO_THRESHOLD_WIDTH = 2560;

/** @struct AutoSplitEntry @brief 카메라별 자동 분할 정보 구조체 */
struct AutoSplitEntry {
  std::string_view cameraId;
  int tileCount;
};

/** @brief 카메라 ID에 따른 타일 분할 맵 정의 (배포 시 설정) */
constexpr std::array<AutoSplitEntry, 0> AUTO_SPLIT_CAMERA_ID_TILE_MAP{};

/** @brief 카메라 ID에 매칭되는 타일 수를 반환 (기본값: 1) */
constexpr int autoSplitTileCountForCameraId(std::string_view cameraId) {
  for (const auto &entry : AUTO_SPLIT_CAMERA_ID_TILE_MAP) {
    if (entry.cameraId == cameraId) {
      return entry.tileCount;
    }
  }
  return 1;
}

/**
 * @class Observable
 * @brief 값이 변경될 때 시그널을 발생시키는 속성 래퍼 템플릿
 * @tparam T 추적할 데이터 타입
 */
template <typename T> class Observable {
public:
  sigslot::signal<T> on_changed; /**< 값이 변경되었을 때 발생하는 시그널 */
  Observable &operator=(const T &t) {
    if (t_ != t) {
      t_ = t;
      on_changed(t);
    }
    return *this;
  }
  /** @return const T& 현재 저장된 값 반환 */
  const T &get() const { return t_; }
  operator const T &() const { return t_; }

private:
  T t_{};
};

/**
 * @class AppState
 * @brief 애플리케이션의 전역 상태(사용자 권한 등) 관리 싱글톤
 */
class AppState {
public:
  static AppState &getInstance() {
    static AppState app;
    return app;
  }
  /** @enum User @brief 사용자 로그인 권한 유형 */
  enum class User : uint8_t { LogOut, Admin, Normal };
  User User_{User::LogOut}; /**< 현재 로그인된 사용자의 권한 상태 */
};
} // namespace Config

/**
 * @section Workflow Guide
 *
 * **[Config 활용 및 관리 원칙]**
 *
 * 1. 설정 우선순위:
 *    - 프로젝트 가동 시 `ExternSettings`를 통해 `Settings.yaml`을 먼저
 * 참조하십시오.
 *    - 파일이 없거나 오류 발생 시 본 파일에 정의된 `constexpr` 상수를
 * 사용하도록 로직을 구성합니다.
 *
 * 2. 상태 전파:
 *    - UI 요소와 바인딩이 필요한 전역 변수는 `Observable<T>`를 사용하여
 * 정의하십시오.
 *    - 이를 통해 명시적인 갱신 루프 없이도 시그널-슬롯 방식으로 UI를 자동
 * 업데이트할 수 있습니다.
 *
 * 3. 상수 추가:
 *    - 시스템 전체에 영향을 주는 매직 넘버는 반드시 본 파일의 적절한
 * 카테고리(Network, Video 등) 아래에 상수로 정의하여 관리하십시오.
 */
