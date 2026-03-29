// ============================================================
//  Core.cpp — Application orchestrator (Refactored)
//
//  Construction order:
//    Layer 1: ThreadPool → NetworkManager → CameraManager → Bridges → ServerConnect
//    Layer 2: Qt Models (view only)
//    Wiring:  wireSignals()
//    QML:     registerContextProperties()
// ============================================================

#include "Core.hpp"

// ── Layer 1 ──────────────────────────────────────────────────────────────────
#include "Src/Config.hpp"
#include "Src/Domain/StatusManager.hpp"
#include "Src/Domain/AlarmManager.hpp"
#include "Src/Network/NetworkEngine.hpp"
#include "Src/Thread/ThreadPool.hpp"
#include <utility>

// ── Layer 2 ──────────────────────────────────────────────────────────────────
#include "Qt/Back/Controllers/AlarmController.hpp"
#include "Qt/Back/Controllers/AppController.hpp"
#include "Qt/Back/Controllers/AuthController.hpp"
#include "Qt/Back/Controllers/SettingsController.hpp"
#include "Qt/Back/Models/AiImageModel.hpp"
#include "Qt/Back/Models/CameraModel.hpp"
#include "Qt/Back/Models/DeviceModel.hpp"
#include "Qt/Back/Models/UserModel.hpp"
#include "Qt/Back/Services/ServerStatus.hpp"
#include "Qt/Back/Services/VideoStream.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QMetaType>
#include <QQmlContext>
#include <nlohmann/json.hpp>

VideoManager *videoManager = nullptr;

// ─────────────────────────────────────────────────────────────────────────────

Core::Core() = default;
Core::~Core() = default;

void Core::init(QQmlEngine &engine) {
  constructLayer1();
  constructLayer2(engine);
  wireSignals();
  registerContextProperties(engine);
}

void Core::shutdown() {
  if (videoManager_) {
    videoManager_->clearAll();
    videoManager = nullptr;
  }

  if (serverConnect_)
    serverConnect_->stop();

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
  threadPool_ = std::make_unique<ThreadPool>();

  // 신규 네트워크 스택
  networkManager_ = std::make_unique<NetworkManager>();
  cameraManager_ = std::make_unique<CameraManager>();
  statusManager_ = std::make_unique<StatusManager>();

  // Bridges — NetworkManager 시그널을 구독하여 Domain 모델 업데이트
  cameraBridge_ = std::make_unique<CameraBridge>(*cameraManager_, *networkManager_);
  authBridge_ = std::make_unique<AuthBridge>(*networkManager_, *statusManager_);

  // ServerConnect — 서버 연결 (Config에서 읽어온 IP/Port)
  auto& settings = Config::ExternSettings::getInstance().ReadSettings_;
  serverConnect_ = std::make_shared<ServerConnect>(
      *networkManager_,
      settings.getServerIP(),
      static_cast<uint16_t>(std::stoi(settings.getServerPort()))
  );
  serverConnect_->Read();
  serverConnect_->run(threadPool_.get());

  // AlarmManager
  alarmManager_ = std::make_unique<AlarmManager>(*threadPool_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 2 — Layer 2: QObject adapters, parented to engine
// ─────────────────────────────────────────────────────────────────────────────
void Core::constructLayer2(QQmlEngine &engine) {
  QObject *parent = &engine;

  settingsController_ = new SettingsController(parent);

  settingsController_ = new SettingsController(parent);

  // Auth controllers — ServerConnect 기반으로 전환 예정 (현재 bridge 매개변수 제거됨)
  login_ = new LoginController(
      settingsController_->serverIp(),
      QString::fromStdString(std::string(Config::SERVER_PORT)), parent);
  signup_ = new SignupController(
      settingsController_->serverIp(),
      QString::fromStdString(std::string(Config::SERVER_PORT)), parent);

  // Models: pure QAbstractListModel views
  cameraModel_ = new CameraModel(parent);
  deviceModel_ = new DeviceModel(parent);
  deviceModel_->setCameraManager(cameraManager_.get());
  serverStatus_ = new ServerStatusModel(parent);
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
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 3 — Signal wiring
// ─────────────────────────────────────────────────────────────────────────────
void Core::wireSignals() {
  qRegisterMetaType<SlotInfo>("SlotInfo");
  qRegisterMetaType<QList<SlotInfo>>("QList<SlotInfo>");

  qRegisterMetaType<QList<SlotInfo>>("QList<SlotInfo>");

  // ── CameraModel → VideoManager (URL 동기화) ─────────────────
  if (cameraModel_ && videoManager_) {
    QObject::connect(cameraModel_, &CameraModel::slotsUpdated, videoManager_,
                     &VideoManager::registerSlots);
    QObject::connect(cameraModel_, &CameraModel::cameraIdsUpdated, videoManager_,
                     &VideoManager::registerCameraIds);
    QObject::connect(cameraModel_, &CameraModel::cameraOnline, videoManager_,
                     &VideoManager::restartWorker);
  }

  if (login_) {
    QObject::connect(login_, &LoginController::logoutRequested, videoManager_,
                     &VideoManager::clearAll);
    QObject::connect(login_, &LoginController::logoutRequested, cameraModel_,
                     &CameraModel::clearAll);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 4 — Register context properties
// ─────────────────────────────────────────────────────────────────────────────
void Core::registerContextProperties(QQmlEngine &engine) {
  auto *ctx = engine.rootContext();

  // QML name             C++ object
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
