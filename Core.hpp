#pragma once

// ============================================================
//  Core.hpp — Application orchestrator (Refactored)
//
//  Layer 1: ThreadPool → NetworkManager → CameraManager → Bridges
//  Layer 2: Qt Models (CameraModel, etc.) — view only
// ============================================================

#include <QMetaObject>
#include <QQmlEngine>
#include <functional>
#include <memory>
#include <string>

// ── Layer 1 forward declarations (pure C++, no Qt headers) ───────────────────
class ThreadPool;
class NetworkManager;
class CameraManager;
class CameraBridge;
class AuthBridge;
class AlarmManager;
class StatusManager;

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
class ServerConnect;

class Core {
public:
  Core();
  ~Core();

  void init(QQmlEngine &engine);
  void shutdown();
  void submitTask(std::function<void()> task);

private:
  void constructLayer1();
  void constructLayer2(QQmlEngine &engine);
  void wireSignals();
  void registerContextProperties(QQmlEngine &engine);

  // ── Layer 1 — owned here ──────────────────────────────────────────────
  std::unique_ptr<ThreadPool> threadPool_;
  std::unique_ptr<NetworkManager> networkManager_;
  std::unique_ptr<CameraManager> cameraManager_;
  std::unique_ptr<StatusManager> statusManager_;
  std::unique_ptr<CameraBridge> cameraBridge_;
  std::unique_ptr<AuthBridge> authBridge_;
  std::unique_ptr<AlarmManager> alarmManager_;
  std::shared_ptr<ServerConnect> serverConnect_;

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
