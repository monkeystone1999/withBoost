// ============================================================
//  Core.cpp — Application orchestrator implementation
//
//  Construction order (strict — do not reorder):
//    Layer 1:  ThreadPool → NetworkService → Stores → AlarmDispatcher
//    Layer 2:  NetworkBridge → Login → Signup
//              → CameraModel → DeviceModel → ServerStatusModel → UserModel
//              → VideoManager → AlarmController
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
#include "Qt/Back/Services/NetworkBridge.hpp"
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
  // ThreadPool first — every other Layer-1 object may submit tasks to it
  threadPool_ = std::make_unique<ThreadPool>();

  // Network infrastructure
  networkService_ = std::make_unique<NetworkService>();

  // Domain stores — hold in-memory snapshots, parse on ThreadPool
  cameraStore_ = std::make_unique<CameraStore>();
  deviceStore_ = std::make_unique<DeviceStore>();
  serverStatusStore_ = std::make_unique<ServerStatusStore>();

  // AlarmDispatcher needs ThreadPool reference — construct last in L1
  alarmDispatcher_ = std::make_unique<AlarmDispatcher>(*threadPool_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Step 2 — Layer 2: QObject adapters, parented to engine
// ─────────────────────────────────────────────────────────────────────────────
void Core::constructLayer2(QQmlEngine &engine) {
  QObject *parent = &engine; // engine is QObject — all adapters become children

  settingsController_ = new SettingsController(parent);

  // NetworkBridge: type bridge, receives INetworkService* (non-owning)
  networkBridge_ = new NetworkBridge(networkService_.get(), parent);

  // Auth controllers: receive NetworkBridge* + server address from
  // SettingsController
  login_ = new LoginController(
      networkBridge_, settingsController_->serverIp(),
      QString::fromStdString(std::string(Config::SERVER_PORT)), parent);
  signup_ = new SignupController(
      networkBridge_, settingsController_->serverIp(),
      QString::fromStdString(std::string(Config::SERVER_PORT)), parent);

  // Models: pure QAbstractListModel views — no parsing, no JSON
  cameraModel_ = new CameraModel(parent);
  deviceModel_ = new DeviceModel(parent);
  serverStatus_ = new ServerStatusModel(parent);
  userModel_ = new UserModel(parent);
  aiImageModel_ = new AiImageModel(parent);

  // VideoManager: manages FFmpeg worker threads per RTSP URL
  videoManager_ = new VideoManager(parent);
  videoManager_->setUrlProvider([this](const QString &cid) {
    return QString::fromStdString(
        cameraStore_->getCameraStruct(cid.toStdString()).sourceUrl);
  });
  videoManager_->setFpsProvider(
      [this]() { return settingsController_->fps(); });
  QObject::connect(
      settingsController_, &SettingsController::fpsChanged, videoManager_,
      [this]() { videoManager_->setFpsLimit(settingsController_->fps()); });
  videoManager = videoManager_; // Set global pointer for VideoStream Bridge

  appController_ = new AppController(parent);
  appController_->setEngine(&engine);

  alarmController_ = new AlarmController(parent);
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
        bindStoreToModel(cameraStore_.get(), cameraModel_, s,
                         &CameraStore::updateFromJson,
                         &CameraModel::onStoreUpdated);

        // DeviceStore parse (Integrated) → DeviceModel update (GUI thread)
        bindStoreToModel(deviceStore_.get(), deviceModel_, s,
                         &DeviceStore::updateFromCameraJson,
                         &DeviceModel::onStoreUpdated);
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
        // ── Integrated update: AVAILABLE info merged into DeviceStore ──
        bindStoreToModel(deviceStore_.get(), deviceModel_, s,
                         &DeviceStore::updateFromAvailableJson,
                         &DeviceModel::onStoreUpdated);

        // ── Legacy update for ServerStatusModel (Dashboard top info) ──
        bindStoreToModel(serverStatusStore_.get(), serverStatus_, s,
                         &ServerStatusStore::updateFromJson,
                         &ServerStatusModel::onStoreUpdated);
      },
      Qt::DirectConnection);

  // ── META (0x09) → CameraStore (sensor_batch) ───────────────────────────
  QObject::connect(
      networkBridge_, &NetworkBridge::metaResultReceived, networkBridge_,
      [this](const QString &json) {
        const std::string s = json.toStdString();
        threadPool_->Submit([this, s] {
          try {
            auto parsed = nlohmann::json::parse(s, nullptr, false);
            if (!parsed.is_object())
              return;

            // Read ip field (primary key)
            std::string ip = parsed.value("ip", "");
            if (ip.empty())
              return;

            if (parsed.contains("sensor_batch") &&
                parsed["sensor_batch"].is_array() &&
                !parsed["sensor_batch"].empty()) {
              auto latest = parsed["sensor_batch"].back();
              DeviceInfo info;
              info.hum = latest.value("hum", 0.0);
              info.light = latest.value("light", 0.0);
              info.tilt = latest.value("tilt", 0.0);
              info.tmp = latest.value("tmp", 0.0);

              // Find all CameraIDs matching this IP
              auto snap = cameraStore_->snapshot();
              for (const auto &cam : snap) {
                // cam.cameraId format: "IP/streamNum"
                if (cam.cameraId.rfind(ip, 0) == 0) {
                  cameraStore_->updateDeviceInfo(cam.cameraId, info);

                  QMetaObject::invokeMethod(
                      cameraModel_,
                      [this, cid = cam.cameraId, info]() {
                        cameraModel_->updateSensorInfo(
                            QString::fromStdString(cid), info.hum, info.light,
                            info.tilt, info.tmp);
                      },
                      Qt::QueuedConnection);
                }
              }
            }
          } catch (...) {
          }
        });
      },
      Qt::DirectConnection);

  // ── AI (0x06) → AlarmDispatcher + CameraStore (person_count) ──────────
  //
  // When:  AI event packet from server.
  // Format: {"device_id":"SubPi_IP","ip":"IP","person_count":N}
  // Why:   Routes person_count to CameraStore.updateAiInfo, and alarm events
  //        to AlarmDispatcher.
  QObject::connect(
      networkBridge_, &NetworkBridge::aiResultReceived, networkBridge_,
      [this](const QString &json) {
        const std::string s = json.toStdString();

        // Route to AlarmDispatcher (existing behavior)
        alarmDispatcher_->dispatch(s, [this](AlarmEvent ev) {
          QMetaObject::invokeMethod(
              alarmController_,
              [this, ev = std::move(ev)]() {
                alarmController_->onAlarm(std::move(ev));
              },
              Qt::QueuedConnection);
        });

        // Route person_count to CameraStore
        threadPool_->Submit([this, s] {
          try {
            auto parsed = nlohmann::json::parse(s, nullptr, false);
            if (!parsed.is_object())
              return;

            std::string ip = parsed.value("ip", "");
            if (ip.empty())
              return;

            if (parsed.contains("person_count")) {
              int count = parsed.value("person_count", 0);

              // Find all CameraIDs matching this IP
              auto snap = cameraStore_->snapshot();
              for (const auto &cam : snap) {
                if (cam.cameraId.rfind(ip, 0) == 0) {
                  AiInfo aiInfo;
                  aiInfo.personCount.push_back(count);
                  cameraStore_->updateAiInfo(cam.cameraId, aiInfo);
                }
              }
            }
          } catch (...) {
          }
        });
      },
      Qt::DirectConnection);

  // ── IMAGE (0x0A) → AiImageModel (GUI Thread via QMetaObject later) ──
  // Contains JSON metadata + binary JPEG payload. We must extract the JSON
  // string.
  QObject::connect(
      networkBridge_, &NetworkBridge::imageResultReceived, networkBridge_,
      [this](const std::vector<uint8_t> &data) {
        // Find the null terminator which separates JSON from JPEG
        auto it = std::find(data.begin(), data.end(), '\0');
        if (it != data.end()) {
          std::string jsonStr(data.begin(), it);
          QByteArray jpegData(reinterpret_cast<const char *>(&*it + 1),
                              std::distance(it + 1, data.end()));

          // Parse IMAGE metadata + resolve IP → CameraID
          threadPool_->Submit([this, jsonStr = std::move(jsonStr),
                               jpegData = std::move(jpegData)] {
            try {
              auto parsed = nlohmann::json::parse(jsonStr);
              std::string ip = parsed.value("ip", "");
              if (ip.empty())
                return;

              QString deviceName =
                  QString::fromStdString(parsed.value("device_id", ""));
              int trackId = parsed.value("track_id", 0);
              int frameIndex = parsed.value("frame_index", 0);
              int totalFrames = parsed.value("total_frames", 0);
              long long timestamp = parsed.value("timestamp_ms", 0LL);

              // Resolve IP → CameraID(s)
              auto snap = cameraStore_->snapshot();
              std::vector<std::string> matchedIds;
              for (const auto &cam : snap) {
                if (cam.cameraId.rfind(ip, 0) == 0) {
                  matchedIds.push_back(cam.cameraId);
                }
              }
              // Fallback: if no match, use ip/0
              if (matchedIds.empty()) {
                matchedIds.push_back(ip + "/0");
              }

              // Base64 encode in ThreadPool (current thread)
              QString base64 = "data:image/jpeg;base64," + jpegData.toBase64();

              for (const auto &cid : matchedIds) {
                QString qCid = QString::fromStdString(cid);
                QMetaObject::invokeMethod(
                    aiImageModel_,
                    [this, qCid, deviceName, trackId, frameIndex, totalFrames,
                     timestamp, b64 = std::move(base64)] {
                      aiImageModel_->onImageReceivedBase64(
                          qCid, deviceName, trackId, frameIndex, totalFrames,
                          timestamp, b64);
                    },
                    Qt::QueuedConnection);
              }
            } catch (const std::exception &e) {
            }
          });
        }
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
  QObject::connect(cameraModel_, &CameraModel::cameraIdsUpdated, videoManager_,
                   &VideoManager::registerCameraIds);

  // F-2: When a camera comes back online, restart its stream worker
  QObject::connect(cameraModel_, &CameraModel::cameraOnline, videoManager_,
                   &VideoManager::restartWorker);

  // ── Logout ──────────────────────────────────────────────────────────────
  // Qt Models (GUI thread — direct slot connections)
  QObject::connect(login_, &LoginController::logoutRequested, videoManager_,
                   &VideoManager::clearAll);
  QObject::connect(login_, &LoginController::logoutRequested, cameraModel_,
                   &CameraModel::clearAll);
  QObject::connect(login_, &LoginController::logoutRequested, deviceModel_,
                   &DeviceModel::clearAll);
  QObject::connect(login_, &LoginController::logoutRequested, alarmController_,
                   &AlarmController::clearAll);
  QObject::connect(login_, &LoginController::logoutRequested, aiImageModel_,
                   &AiImageModel::clearAll);
  // Backend Stores (thread-safe, called from GUI thread)
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
  ctx->setContextProperty("aiImageModel", aiImageModel_);
  ctx->setContextProperty("videoManager", videoManager_);

  ctx->setContextProperty("appController", appController_);
  ctx->setContextProperty("alarmController", alarmController_);
  ctx->setContextProperty("settingsController", settingsController_);
}
