/**
 * @file Core.hpp
 * @brief 애플리케이션 전체 로직 조율 및 레이어 간 오케스트레이터
 *
 * 이 파일은 시스템의 초기화, 종료, 그리고 각 모듈(네트워크, 도메인, UI 모델)
 * 간의 데이터 흐름을 총괄하는 Core 클래스를 정의합니다.
 */

#pragma once

#include <QMetaObject>
#include <QQmlEngine>
#include <functional>
#include <memory>
#include <string>

// ── Layer 1 forward declarations (pure C++, no Qt headers) ───────────────────
class ThreadEngine;
class NetworkManager;
class CameraManager;
class CameraBridge;
class AuthBridge;
class AlarmManager;
class StatusManager;
class TextConnect;

// ── Layer 2 forward declarations (QObject adapters) ──────────────────────────
class LoginController;
class SignupController;
class AppController;
class AlarmController;
class SettingsController;
class CameraModel;
class DeviceModel;
class ServerStatusModel;
class UserModel;
class VideoManager;
class AiImageModel;
class AlarmService;

/**
 * @class Core
 * @brief 프로젝트의 핵심 엔진이자 모듈 간 결합을 제어하는 오케스트레이터 클래스
 *
 * **Standard Usage Methodology:**
 * 1. App.cpp에서 메인 이벤트 루프 돌입 전 init()을 호출하여 레이어 1, 2를
 * 구성합니다.
 * 2. QML 엔진에 주요 컨트롤러들을 컨텍스트 프로퍼티로 주입하여 UI 바인딩을
 * 활성화합니다.
 * 3. shutdown()을 통해 시스템 종료 시 모든 비동기 작업과 자원을 안전하게
 * 정리합니다.
 */
class Core {
public:
  Core();
  ~Core();

  /**
   * @brief 시스템 초기화 및 레이어 구성
   * @param engine QML 실행 엔진 객체
   */
  void init(QQmlEngine &engine);

  /** @brief 전역 자원 해제 및 비동기 엔진 정지 */
  void shutdown();

  /**
   * @brief 워킹 스레드 풀에 범용 작업 등록
   * @param task 실행할 함수 객체
   */
  void submitTask(std::function<void()> task);

private:
  /** @brief [Layer 1] 순수 C++ 도메인 및 네트워크 엔진 인스턴스화 */
  void constructLayer1();
  /** @brief [Layer 2] Qt 어댑터 및 상위 컨트롤러 인스턴스화 */
  void constructLayer2(QQmlEngine &engine);
  /** @brief 레이어 간 이벤트 통지를 위한 시그널 연동 */
  void wireSignals();
  /** @brief QML 레이어에서 접근 가능한 프로퍼티 등록 */
  void registerContextProperties(QQmlEngine &engine);

  // ── Layer 1 — owned here ──────────────────────────────────────────────
  std::unique_ptr<ThreadEngine> threadPool_;       /**< 범용 스레드 풀 */
  std::unique_ptr<NetworkManager> networkManager_; /**< 네트워크 통합 관리 */
  std::unique_ptr<CameraManager> cameraManager_;   /**< 카메라 데이터 관리 */
  std::unique_ptr<StatusManager> statusManager_;   /**< 시스템 상태 관리 */
  std::unique_ptr<CameraBridge> cameraBridge_;     /**< 카메라 데이터 중계 */
  std::unique_ptr<AuthBridge> authBridge_;         /**< 인증 데이터 중계 */
  std::unique_ptr<AlarmManager> alarmManager_;     /**< 알람 이벤트 관리 */
  std::shared_ptr<TextConnect> serverConnect_;     /**< 서버 TCP 커넥션 */

  // ── Layer 2 — parented to QQmlEngine ──────────────────────────────────
  LoginController *login_ = nullptr;
  SignupController *signup_ = nullptr;
  CameraModel *cameraModel_ = nullptr;
  DeviceModel *deviceModel_ = nullptr;
  ServerStatusModel *serverStatus_ = nullptr;
  UserModel *userModel_ = nullptr;
  VideoManager *videoManager_ = nullptr;
  AiImageModel *aiImageModel_ = nullptr;

  AppController *appController_ = nullptr;
  AlarmController *alarmController_ = nullptr;
  SettingsController *settingsController_ = nullptr;
  AlarmService *alarmService_ = nullptr;

  // ── Generic Cross-Thread Binding ─────────────────────────────────────
  /*
  template <typename Store, typename Model, typename ParsedData>
  void
  bindStoreToModel(Store *store, Model *model, const std::string &json,
                   void (Store::*parseFunc)(const std::string &,
                                            std::function<void(ParsedData)>),
                   void (Model::*updateFunc)(ParsedData)) {
    submitTask([=]() {
      (store->*parseFunc)(json, [=](ParsedData data) {
        QMetaObject::invokeMethod(
            model,
            [=, d = std::move(data)]() mutable {
              (model->*updateFunc)(std::move(d));
            },
            Qt::QueuedConnection);
      });
    });
  }
  */
};
