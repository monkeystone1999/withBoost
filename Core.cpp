// ============================================================
//  Core.cpp — Application orchestrator implementation
//
//  Construction order (strict — do not reorder):
//    Layer 1:  ThreadPool → NetworkService → Stores → AlarmDispatcher
//    Layer 2:  NetworkBridge → Login → Signup
//              → CameraModel → DeviceModel → ServerStatusModel
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
#include "Src/Domain/AlarmDispatcher.hpp"
#include "Src/Domain/AlarmEvent.hpp"
#include "Src/Domain/CameraStore.hpp"
#include "Src/Domain/DeviceStore.hpp"
#include "Src/Domain/ServerStatusStore.hpp"
#include "Src/Network/NetworkService.hpp"
#include "Src/Thread/ThreadPool.hpp"

// ── Layer 2 ──────────────────────────────────────────────────────────────────
#include "Qt/Back/AlarmManager.hpp"
#include "Qt/Back/CameraModel.hpp"
#include "Qt/Back/DeviceModel.hpp"
#include "Qt/Back/Login.hpp"
#include "Qt/Back/NetworkBridge.hpp"
#include "Qt/Back/ServerStatusModel.hpp"
#include "Qt/Back/Signup.hpp"
#include "Qt/Back/VideoManager.hpp"

#include <QDebug>
#include <QMetaType>
#include <QMetaObject>
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
  if (m_videoManager)
    m_videoManager->clearAll();

  qDebug() << "[Core] shutdown — disconnecting network";
  if (m_networkService)
    m_networkService->disconnect();

  // Layer 1 unique_ptrs destroy in reverse declaration order
  // (alarmDispatcher → serverStatusStore → deviceStore →
  //  cameraStore → networkService → threadPool)
  m_alarmDispatcher.reset();
  m_serverStatusStore.reset();
  m_deviceStore.reset();
  m_cameraStore.reset();
  m_networkService.reset();
  m_threadPool.reset();

  qDebug() << "[Core] shutdown complete";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 1 — Layer 1: pure C++ objects, no Qt
// ─────────────────────────────────────────────────────────────────────────────
void Core::constructLayer1() {
  // ThreadPool first — every other Layer-1 object may submit tasks to it
  m_threadPool = std::make_unique<ThreadPool>(Config::THREAD_POOL_SIZE);

  // Network infrastructure
  m_networkService = std::make_unique<NetworkService>();

  // Domain stores — hold in-memory snapshots, parse on ThreadPool
  m_cameraStore = std::make_unique<CameraStore>();
  m_deviceStore = std::make_unique<DeviceStore>();
  m_serverStatusStore = std::make_unique<ServerStatusStore>();

  // AlarmDispatcher needs ThreadPool reference — construct last in L1
  m_alarmDispatcher = std::make_unique<AlarmDispatcher>(*m_threadPool);

  qDebug() << "[Core] Layer 1 constructed";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 2 — Layer 2: QObject adapters, parented to engine
// ─────────────────────────────────────────────────────────────────────────────
void Core::constructLayer2(QQmlEngine &engine) {
  QObject *parent = &engine; // engine is QObject — all adapters become children

  // NetworkBridge: type bridge, receives INetworkService* (non-owning)
  m_networkBridge = new NetworkBridge(m_networkService.get(), parent);

  // Auth controllers: receive NetworkBridge* + server address from Config
  m_login = new Login(
      m_networkBridge, QString::fromStdString(std::string(Config::SERVER_HOST)),
      QString::fromStdString(std::string(Config::SERVER_PORT)), parent);
  m_signup = new Signup(
      m_networkBridge, QString::fromStdString(std::string(Config::SERVER_HOST)),
      QString::fromStdString(std::string(Config::SERVER_PORT)), parent);

  // Models: pure QAbstractListModel views — no parsing, no JSON
  m_cameraModel = new CameraModel(parent);
  m_deviceModel = new DeviceModel(parent);
  m_serverStatus = new ServerStatusModel(parent);

  // VideoManager: manages FFmpeg worker threads per RTSP URL
  m_videoManager = new VideoManager(parent);

  // AlarmManager: receives pre-parsed AlarmEvent from Core wiring
  m_alarmManager = new AlarmManager(m_alarmDispatcher.get(), parent);

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
      m_networkBridge, &NetworkBridge::cameraListReceived,
      m_networkBridge, // context object for lifetime
      [this](const QString &json) {
        const std::string s = json.toStdString();

        // CameraStore parse → CameraModel update (GUI thread)
        m_threadPool->submit([this, s] {
          m_cameraStore->updateFromJson(
              s, [this](std::vector<CameraData> snap) {
                QMetaObject::invokeMethod(
                    m_cameraModel,
                    [this, snap = std::move(snap)]() mutable {
                      m_cameraModel->onStoreUpdated(std::move(snap));
                    },
                    Qt::QueuedConnection);
              });
        });

        // DeviceStore parse (SUB_PI filter) → DeviceModel update (GUI thread)
        m_threadPool->submit([this, s] {
          m_deviceStore->updateFromJson(
              s, [this](std::vector<DeviceData> snap) {
                QMetaObject::invokeMethod(
                    m_deviceModel,
                    [this, snap = std::move(snap)]() mutable {
                      m_deviceModel->onStoreUpdated(std::move(snap));
                    },
                    Qt::QueuedConnection);
              });
        });
      },
      Qt::DirectConnection);

  // ── DEVICE (0x04) → ServerStatusStore on ThreadPool ──────────────────
  //
  // When:  Every DEVICE packet (5 s interval, server side).
  // Why:   JSON with cpu/mem/temp arrays must not block GUI.
  // How:   Same submit pattern as CAMERA above.
  QObject::connect(
      m_networkBridge, &NetworkBridge::deviceStatusReceived, m_networkBridge,
      [this](const QString &json) {
        const std::string s = json.toStdString();
        m_threadPool->submit([this, s] {
          m_serverStatusStore->updateFromJson(s, [this](ServerStatusData data) {
            QMetaObject::invokeMethod(
                m_serverStatus,
                [this, d = std::move(data)]() mutable {
                  m_serverStatus->onStoreUpdated(std::move(d));
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
      m_networkBridge, &NetworkBridge::aiResultReceived, m_networkBridge,
      [this](const QString &json) {
        m_alarmDispatcher->dispatch(json.toStdString(), [this](AlarmEvent ev) {
          QMetaObject::invokeMethod(
              m_alarmManager,
              [this, ev = std::move(ev)]() {
                m_alarmManager->onAlarm(std::move(ev));
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
  QObject::connect(m_cameraModel, &CameraModel::slotsUpdated, m_videoManager,
                   &VideoManager::registerSlots);
  QObject::connect(m_cameraModel, &CameraModel::urlsUpdated, m_videoManager,
                   &VideoManager::registerUrls);

  qDebug() << "[Core] signals wired";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 4 — Register context properties
// ─────────────────────────────────────────────────────────────────────────────
void Core::registerContextProperties(QQmlEngine &engine) {
  auto *ctx = engine.rootContext();

  // QML name             C++ object
  ctx->setContextProperty("networkBridge", m_networkBridge);
  ctx->setContextProperty("loginController", m_login);
  ctx->setContextProperty("signupController", m_signup);
  ctx->setContextProperty("cameraModel", m_cameraModel);
  ctx->setContextProperty("deviceModel", m_deviceModel);
  ctx->setContextProperty("serverStatus", m_serverStatus);
  ctx->setContextProperty("videoManager", m_videoManager);
  ctx->setContextProperty("alarmManager", m_alarmManager);

  qDebug() << "[Core] context properties registered";
}
