// ============================================================
//  Core.cpp — Application orchestrator implementation
//
//  Construction order (strict — do not reorder):
//    Layer 1:  ThreadPool → NetworkService → Stores → AlarmDispatcher
//    Layer 2:  NetworkBridge → Login → Signup
//              → CameraModel → DeviceModel → ServerStatusModel → UserModel
//              → VideoManager → AlarmManager
//    Wiring:   wireSignals() after all objects exist
//    QML:      registerContextProperties() last
//
//  Shutdown order (reverse):
//    VideoManager::clearAll()  — joins all FFmpeg threads
//    NetworkService::disconnect() — closes socket, joins asio thread
//    Layer 1 unique_ptrs destruct in reverse declaration order
// ============================================================

#include "Core.hpp"

// ── Layer 1 ──────────────────────────────────────────────────────────────────
#include "Src/Config.hpp"
#include "Src/Domain/Alarm.hpp"
#include "Src/Domain/Camera.hpp"
#include "Src/Domain/Device.hpp"
#include "Src/Domain/ServerStatus.hpp"
#include "Src/Network/NetworkService.hpp"
#include "Src/Thread/ThreadPool.hpp"

// ── Layer 2 ──────────────────────────────────────────────────────────────────
#include "Qt/Back/AlarmController.hpp"
#include "Qt/Back/AlarmManager.hpp"
#include "Qt/Back/AppController.hpp"
#include "Qt/Back/Auth.hpp"
#include "Qt/Back/Camera.hpp"
#include "Qt/Back/Device.hpp"
#include "Qt/Back/NetworkBridge.hpp"
#include "Qt/Back/ServerStatus.hpp"
#include "Qt/Back/SettingsController.hpp"
#include "Qt/Back/UserModel.hpp"
#include "Qt/Back/VideoStream.hpp"


#include <QDebug>
#include <QMetaObject>
#include <QMetaType>
#include <QQmlContext>

// ─────────────────────────────────────────────────────────────────────────────

Core::Core() = default;
Core::~Core() = default;

void Core::init(QQmlEngine &engine) {
  constructLayer1();
  constructLayer2(engine);
  wireSignals();
  registerContextProperties(engine);
  qDebug() << "[Core] init complete";
}

void Core::shutdown() {
  qDebug() << "[Core] shutdown — stopping video streams";
  if (videoManager_)
    videoManager_->clearAll();

  qDebug() << "[Core] shutdown — disconnecting network";
  if (networkService_)
    networkService_->disconnect();

  // Layer 1 unique_ptrs destroy in reverse declaration order
  // (alarmDispatcher → serverStatusStore → deviceStore →
  //  cameraStore → networkService → threadPool)
  alarmDispatcher_.reset();
  serverStatusStore_.reset();
  deviceStore_.reset();
  cameraStore_.reset();
  networkService_.reset();
  threadPool_.reset();

  qDebug() << "[Core] shutdown complete";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 1 — Layer 1: pure C++ objects, no Qt
// ─────────────────────────────────────────────────────────────────────────────
void Core::constructLayer1() {
  // ThreadPool first — every other Layer-1 object may submit tasks to it
  threadPool_ = std::make_unique<ThreadPool>(Config::THREAD_POOL_SIZE);

  // Network infrastructure
  networkService_ = std::make_unique<NetworkService>();

  // Domain stores — hold in-memory snapshots, parse on ThreadPool
  cameraStore_ = std::make_unique<CameraStore>();
  deviceStore_ = std::make_unique<DeviceStore>();
  serverStatusStore_ = std::make_unique<ServerStatusStore>();

  // AlarmDispatcher needs ThreadPool reference — construct last in L1
  alarmDispatcher_ = std::make_unique<AlarmDispatcher>(*threadPool_);

  qDebug() << "[Core] Layer 1 constructed";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 2 — Layer 2: QObject adapters, parented to engine
// ─────────────────────────────────────────────────────────────────────────────
void Core::constructLayer2(QQmlEngine &engine) {
  QObject *parent = &engine; // engine is QObject — all adapters become children

  // NetworkBridge: type bridge, receives INetworkService* (non-owning)
  networkBridge_ = new NetworkBridge(networkService_.get(), parent);

  // Auth controllers: receive NetworkBridge* + server address from Config
  login_ = new LoginController(
      networkBridge_, QString::fromStdString(std::string(Config::SERVER_HOST)),
      QString::fromStdString(std::string(Config::SERVER_PORT)), parent);
  signup_ = new SignupController(
      networkBridge_, QString::fromStdString(std::string(Config::SERVER_HOST)),
      QString::fromStdString(std::string(Config::SERVER_PORT)), parent);

  // Models: pure QAbstractListModel views — no parsing, no JSON
  cameraModel_ = new CameraModel(parent);
  deviceModel_ = new DeviceModel(parent);
  serverStatus_ = new ServerStatusModel(parent);
  userModel_ = new UserModel(parent);

  // VideoManager: manages FFmpeg worker threads per RTSP URL
  videoManager_ = new VideoManager(parent);

  // AlarmManager: receives pre-parsed AlarmEvent from Core wiring
  alarmManager_ = new AlarmManager(alarmDispatcher_.get(), parent);

  appController_ = new AppController(parent);
  alarmController_ = new AlarmController(parent);
  settingsController_ = new SettingsController(parent);
  settingsController_->load();

  qDebug() << "[Core] Layer 2 constructed";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 3 — Signal wiring
//
//  Rule: ALL cross-thread callbacks MUST use Qt::QueuedConnection or
//        QMetaObject::invokeMethod(..., Qt::QueuedConnection).
//        Lambdas that cross to the ThreadPool must capture by value only.
// ─────────────────────────────────────────────────────────────────────────────
void Core::wireSignals() {
  qRegisterMetaType<SlotInfo>("SlotInfo");
  qRegisterMetaType<QList<SlotInfo>>("QList<SlotInfo>");

  // ── CAMERA (0x07) → CameraStore + DeviceStore on ThreadPool ──────────
  //
  // When:  Every CAMERA packet (login + 5 s interval).
  // Why:   13 cameras × JSON parse must NOT block the GUI thread.
  // How:   Lambda captured by Qt::DirectConnection (same GUI thread as
  //        NetworkBridge emission). Immediately submits two ThreadPool
  //        tasks — one per store. Each store's callback is posted back
  //        to the GUI thread via QueuedConnection.
  QObject::connect(
      networkBridge_, &NetworkBridge::cameraListReceived,
      networkBridge_, // context object for lifetime
      [this](const QString &json) {
        const std::string s = json.toStdString();

        // CameraStore parse → CameraModel update (GUI thread)
        threadPool_->submit([this, s] {
          cameraStore_->updateFromJson(s, [this](std::vector<CameraData> snap) {
            QMetaObject::invokeMethod(
                cameraModel_,
                [this, snap = std::move(snap)]() mutable {
                  cameraModel_->onStoreUpdated(std::move(snap));
                },
                Qt::QueuedConnection);
          });
        });

        // DeviceStore parse (Integrated) → DeviceModel update (GUI thread)
        threadPool_->submit([this, s] {
          deviceStore_->updateFromCameraJson(
              s, [this](std::vector<DeviceIntegrated> snap) {
                QMetaObject::invokeMethod(
                    deviceModel_,
                    [this, snap = std::move(snap)]() mutable {
                      deviceModel_->onStoreUpdated(std::move(snap));
                    },
                    Qt::QueuedConnection);
              });
        });
      },
      Qt::DirectConnection);

  // ── AVAILABLE (0x05) → DeviceStore on ThreadPool ──────────────────
  //
  // When:  Every AVAILABLE packet (5 s interval).
  // Why:   Device status (cpu/mem/temp) update.
  // How:   Redirected from ServerStatusStore to DeviceStore for IP-based
  // integration.
  QObject::connect(
      networkBridge_, &NetworkBridge::deviceStatusReceived, networkBridge_,
      [this](const QString &json) {
        const std::string s = json.toStdString();
        threadPool_->submit([this, s] {
          // ── Integrated update: AVAILABLE info merged into DeviceStore ──
          deviceStore_->updateFromAvailableJson(
              s, [this](std::vector<DeviceIntegrated> snap) {
                QMetaObject::invokeMethod(
                    deviceModel_,
                    [this, snap = std::move(snap)]() mutable {
                      deviceModel_->onStoreUpdated(std::move(snap));
                    },
                    Qt::QueuedConnection);
              });

          // ── Legacy update for ServerStatusModel (Dashboard top info) ──
          serverStatusStore_->updateFromJson(s, [this](ServerStatusData data) {
            QMetaObject::invokeMethod(
                serverStatus_,
                [this, d = std::move(data)]() mutable {
                  serverStatus_->onStoreUpdated(std::move(d));
                },
                Qt::QueuedConnection);
          });
        });
      },
      Qt::DirectConnection);

  // ── AI (0x06) → AlarmDispatcher (already submits to ThreadPool) ───────
  //
  // When:  AI event packet from server.
  // Why:   AI JSON can be large and complex — never parse on GUI thread.
  // How:   AlarmDispatcher::dispatch() submits to ThreadPool internally.
  //        The callback posts the parsed AlarmEvent back to GUI thread.
  QObject::connect(
      networkBridge_, &NetworkBridge::aiResultReceived, networkBridge_,
      [this](const QString &json) {
        alarmDispatcher_->dispatch(json.toStdString(), [this](AlarmEvent ev) {
          QMetaObject::invokeMethod(
              alarmManager_,
              [this, ev = std::move(ev)]() {
                alarmManager_->onAlarm(std::move(ev));
              },
              Qt::QueuedConnection);
        });
      },
      Qt::DirectConnection);

  // ── CameraModel → VideoManager (URL synchronization) ─────────────────
  //
  // When:  CameraModel emits urlsUpdated after onStoreUpdated.
  // Why:   VideoManager must create exactly one FFmpeg worker per URL.
  //        CameraModel is the authoritative list after login.
  // How:   Direct Qt signal→slot on GUI thread — no threading needed here.
  QObject::connect(cameraModel_, &CameraModel::slotsUpdated, videoManager_,
                   &VideoManager::registerSlots);
  QObject::connect(cameraModel_, &CameraModel::urlsUpdated, videoManager_,
                   &VideoManager::registerUrls);

  // F-2: When a camera comes back online, restart its stream worker
  QObject::connect(cameraModel_, &CameraModel::cameraOnline, videoManager_,
                   &VideoManager::restartWorker);

  // ── Logout ──────────────────────────────────────────────────────────────
  QObject::connect(login_, &LoginController::logoutRequested, videoManager_,
                   &VideoManager::clearAll);
  QObject::connect(login_, &LoginController::logoutRequested, cameraModel_,
                   &CameraModel::clearAll);

  // ── Alarm ───────────────────────────────────────────────────────────────
  QObject::connect(alarmManager_, &AlarmManager::alarmTriggered,
                   alarmController_, &AlarmController::onAlarmTriggered);

  qDebug() << "[Core] signals wired";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 4 — Register context properties
// ─────────────────────────────────────────────────────────────────────────────
void Core::registerContextProperties(QQmlEngine &engine) {
  auto *ctx = engine.rootContext();

  // QML name             C++ object
  ctx->setContextProperty("networkBridge", networkBridge_);
  ctx->setContextProperty("loginController", login_);
  ctx->setContextProperty("signupController", signup_);
  ctx->setContextProperty("cameraModel", cameraModel_);
  ctx->setContextProperty("deviceModel", deviceModel_);
  ctx->setContextProperty("serverStatus", serverStatus_);
  ctx->setContextProperty("userModel", userModel_);
  ctx->setContextProperty("videoManager", videoManager_);
  ctx->setContextProperty("alarmManager", alarmManager_);

  ctx->setContextProperty("appController", appController_);
  ctx->setContextProperty("alarmController", alarmController_);
  ctx->setContextProperty("settingsController", settingsController_);

  // ── Test Data for UserModel ────────────────────────────────────────────
  // TODO: Remove this test data when real user data comes from server
  userModel_->addUser("user001", QStringLiteral("홍길동"), "hong@example.com",
                      "user");
  userModel_->addUser("user002", QStringLiteral("김철수"), "kim@example.com",
                      "user");
  userModel_->addUser("admin001", QStringLiteral("관리자"), "admin@example.com",
                      "admin");
  userModel_->setUserOnline("user001", true);
  userModel_->setUserOnline("user002", true);
  userModel_->setUserOnline("admin001", true);
  userModel_->updateActiveCameras("user001", 3);
  userModel_->updateActiveCameras("user002", 1);
  userModel_->updateActiveCameras("admin001", 0);

  qDebug() << "[Core] context properties registered";
  qDebug() << "[Core] test users added to UserModel";
}
