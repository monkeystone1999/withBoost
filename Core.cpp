/**
 * @file Core.cpp
 * @brief Core 클래스 구현부 - 애플리케이션 수명 주기 및 의존성 주입 관리
 */

#include "Core.hpp"

// ── Layer 1 (C++ Domain) ─────────────────────────────────────────────────────
#include "Src/Config.hpp"
#include "Src/Domain/AlarmManager.hpp"
#include "Src/Domain/AuthBridge.hpp"
#include "Src/Domain/CameraBridge.hpp"
#include "Src/Domain/StatusManager.hpp"
#include "Src/Network/NetworkEngine.hpp"
#include "Src/Network/NetworkManager.hpp"
#include "Src/Thread/ThreadEngine.hpp"
#include <utility>

// ── Layer 2 (Qt Backend) ─────────────────────────────────────────────────────
#include "Qt/Back/Controllers/AlarmController.hpp"
#include "Qt/Back/Controllers/AppController.hpp"
#include "Qt/Back/Controllers/AuthController.hpp"
#include "Qt/Back/Controllers/SettingsController.hpp"
#include "Qt/Back/Models/AiImageModel.hpp"
#include "Qt/Back/Models/CameraModel.hpp"
#include "Qt/Back/Models/DeviceModel.hpp"
#include "Qt/Back/Models/UserModel.hpp"
#include "Qt/Back/Services/AlarmService.hpp"
#include "Qt/Back/Services/ServerStatus.hpp"
#include "Qt/Back/Services/VideoStream.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QMetaType>
#include <QQmlContext>
#include <QTimer>
#include <nlohmann/json.hpp>

/** @brief QML 및 다른 서비스에서 접근 가능한 전역 비디오 매니저 포인터 */
VideoManager *videoManager = nullptr;

Core::Core() = default;
Core::~Core() = default;

void Core::init(QQmlEngine &engine) {
  constructLayer1();
  constructLayer2(engine);
  wireSignals();
  registerContextProperties(engine);
}

void Core::shutdown() {
  // 1. 비디오 매니저 및 타이머 등 UI 관련 자원 먼저 정리
  if (videoManager_) {
    videoManager_->clearAll();
    videoManager = nullptr;
  }

  // 2. 네트워크 연결 중단 및 스레드 정지 (관련 매니저들이 사라지기 전에
  // 완료되어야 함)
  if (serverConnect_) {
    serverConnect_->stop();
  }

  // 3. 스레드 풀 종료 (진행 중인 모든 Task 완료 대기)
  if (threadPool_) {
    threadPool_->Shutdown();
  }

  // 4. 나머지 도메인 매니저들 해제
  authBridge_.reset();
  cameraBridge_.reset();
  alarmManager_.reset();
  serverConnect_.reset();
  statusManager_.reset();
  cameraManager_.reset();
  networkManager_.reset();
  threadPool_.reset();
}

void Core::submitTask(std::function<void()> task) {
  if (threadPool_) {
    threadPool_->Submit(std::move(task));
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 1 — Layer 1: pure C++ objects, no Qt
// ─────────────────────────────────────────────────────────────────────────────
void Core::constructLayer1() {
  threadPool_ = std::make_unique<ThreadEngine>();

  // 신규 네트워크 스택
  networkManager_ = std::make_unique<NetworkManager>();
  cameraManager_ = std::make_unique<CameraManager>();
  statusManager_ = std::make_unique<StatusManager>();

  // Bridges — NetworkManager 시그널을 구독하여 Domain 모델 업데이트
  cameraBridge_ =
      std::make_unique<CameraBridge>(*cameraManager_, *networkManager_);
  authBridge_ = std::make_unique<AuthBridge>(*networkManager_, *statusManager_);

  // TextConnect — 서버 연결 (Config에서 읽어온 IP/Port)
  auto &settings = Config::ExternSettings::getInstance().ReadSettings_;
  serverConnect_ = std::make_shared<TextConnect>(
      *networkManager_, settings.getServerIP(),
      static_cast<uint16_t>(std::stoi(settings.getServerPort())));
  networkManager_->setTextConnect(serverConnect_.get());

  // AlarmManager (Domain)
  alarmManager_ = std::make_unique<AlarmManager>(*threadPool_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 2 — Layer 2: QObject adapters, parented to engine
// ─────────────────────────────────────────────────────────────────────────────
void Core::constructLayer2(QQmlEngine &engine) {
  QObject *parent = &engine;

  settingsController_ = new SettingsController(parent);

  // Auth controllers — ServerConnect 기반으로 전환 예정
  auto &readSettings = Config::ExternSettings::getInstance().ReadSettings_;
  login_ =
      new LoginController(settingsController_->serverIp(),
                          QString::fromStdString(readSettings.getServerPort()),
                          authBridge_.get(), parent);
  signup_ =
      new SignupController(settingsController_->serverIp(),
                           QString::fromStdString(readSettings.getServerPort()),
                           authBridge_.get(), parent);

  // Models: pure QAbstractListModel views
  cameraModel_ = new CameraModel(parent);
  deviceModel_ = new DeviceModel(parent);
  deviceModel_->setCameraManager(cameraManager_.get());
  serverStatus_ = new ServerStatusModel(parent);
  serverStatus_->setStatusManager(statusManager_.get());
  userModel_ = new UserModel(parent);
  aiImageModel_ = new AiImageModel(parent);

  // VideoManager: URL을 CameraManager에서 가져오도록 변경
  videoManager_ = new VideoManager(parent);
  videoManager_->setCameraManager(cameraManager_.get());
  videoManager_->setUrlProvider([this](const QString &cid) {
    /// CameraManager에서 IP/Index로 카메라를 찾아 RTSP URL 반환
    auto camera = cameraManager_->Get(cid.toStdString());
    return camera ? QString::fromStdString(camera->rtsp_) : QString();
  });
  videoManager_->setFpsProvider(
      [this]() { return settingsController_->fps(); });
  QObject::connect(
      settingsController_, &SettingsController::fpsChanged, videoManager_,
      [this]() { videoManager_->setFpsLimit(settingsController_->fps()); });
  videoManager = videoManager_;

  appController_ = new AppController(parent);
  appController_->setEngine(&engine);
  alarmController_ = new AlarmController(parent);
  alarmService_ = new AlarmService(alarmManager_.get(), parent);

  // 폴링 메커니즘: 주기적으로 백엔드 모델 업데이트
  QTimer *pollTimer = new QTimer(parent);
  QObject::connect(pollTimer, &QTimer::timeout, [this]() {
    if (cameraModel_) {
      cameraModel_->refreshFromCameraManager();
      cameraModel_->refreshSensorInfo();
    }
    if (deviceModel_) {
      deviceModel_->refreshFromCameraManager();
    }
    if (serverStatus_) {
      serverStatus_->refreshFromStatusManager();
    }
    if (userModel_) {
      userModel_->refreshFromStatusManager();
    }
  });
  pollTimer->start(1000); // 1초 주기로 모델 동기화
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 3 — Signal wiring
// ─────────────────────────────────────────────────────────────────────────────
void Core::wireSignals() {
  qRegisterMetaType<SlotInfo>("SlotInfo");
  qRegisterMetaType<QList<SlotInfo>>("QList<SlotInfo>");

  // ── Alarm Notification (Core Dispatcher → UI Controller) ──
  if (alarmService_ && alarmController_) {
    QObject::connect(alarmService_, &AlarmService::alarmTriggered,
                     alarmController_, &AlarmController::onAlarm);
  }

  // ── CameraModel → VideoManager (URL 동기화) ─────────────────
  if (cameraModel_ && videoManager_) {
    QObject::connect(cameraModel_, &CameraModel::slotsUpdated, videoManager_,
                     &VideoManager::registerSlots);
    QObject::connect(cameraModel_, &CameraModel::cameraIdsUpdated,
                     videoManager_, &VideoManager::registerCameraIds);
    QObject::connect(cameraModel_, &CameraModel::cameraOnline, videoManager_,
                     &VideoManager::restartWorker);
  }

  if (login_ && authBridge_) {
    authBridge_->onLoginSuccess = [this](std::string state,
                                         std::string username) {
      QMetaObject::invokeMethod(
          login_, "handleLoginSuccess", Qt::QueuedConnection,
          Q_ARG(QString, QString::fromStdString(state)),
          Q_ARG(QString, QString::fromStdString(username)));
    };
    authBridge_->onLoginFailed = [this](std::string error) {
      QMetaObject::invokeMethod(login_, "handleLoginFailed",
                                Qt::QueuedConnection,
                                Q_ARG(QString, QString::fromStdString(error)));
    };

    QObject::connect(login_, &LoginController::logoutRequested, [this]() {
      if (videoManager_)
        videoManager_->clearAll();
      if (cameraModel_)
        cameraModel_->clearAll();
      if (serverConnect_)
        serverConnect_->stop();
    });
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 4 — Register context properties
// ─────────────────────────────────────────────────────────────────────────────
void Core::registerContextProperties(QQmlEngine &engine) {
  auto *ctx = engine.rootContext();

  ctx->setContextProperty("loginController", login_);
  ctx->setContextProperty("signupController", signup_);
  ctx->setContextProperty("cameraModel", cameraModel_);
  ctx->setContextProperty("deviceModel", deviceModel_);
  ctx->setContextProperty("serverStatus", serverStatus_);
  ctx->setContextProperty("userModel", userModel_);
  ctx->setContextProperty("aiImageModel", aiImageModel_);
  ctx->setContextProperty("videoManager", videoManager_);

  ctx->setContextProperty("appController", appController_);
  ctx->setContextProperty("alarmController", alarmController_);
  ctx->setContextProperty("settingsController", settingsController_);
}

/**
 * @section Workflow Guide
 *
 * **[Core 애플리케이션 기동 워크플로우]**
 *
 * 1. 기초 레이어(Layer 1) 구성:
 *    - `ThreadEngine`을 기점으로 네트워크 관리자 및 도메인 매니저들을
 * 생성합니다.
 *    - `Bridge` 객체들이 생성되어 네트워크 신호를 도메인 모델로 전파할 준비를
 * 마칩니다.
 *
 * 2. 통신 시작:
 *    - `TextConnect`(서버 채널)가 기동되어 `ThreadEngine` 워커에서 비동기 I/O
 * 루프를 시작합니다.
 *
 * 3. 어댑터 레이어(Layer 2) 바인딩:
 *    - Qt 기반의 모델들과 컨트롤러들을 생성하고, 도메인 레이어의 포인터를
 * 주입하여 데이터를 UI 형식으로 변환할 준비를 합니다.
 *
 * 4. QML 통합:
 *    - 모든 준비가 끝난 QObject 인스턴스들을 QML 컨텍스트 프로퍼티로 등록하여
 * QML 엔진이 로딩될 때 즉시 사용할 수 있도록 합니다.
 */
