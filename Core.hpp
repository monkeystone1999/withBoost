#pragma once

// ============================================================
//  Core.hpp — Application orchestrator
//
//  WHEN:  Constructed in main(), before engine.loadFromModule().
//  WHERE: Project root, alongside App.cpp.
//  WHY:   App.cpp must only handle Qt engine configuration.
//         All object construction, dependency injection, and signal
//         wiring belongs here — one place, explicit order, no globals.
//  HOW:   Core owns all Layer-1 (pure C++) objects via unique_ptr.
//         Layer-2 QObjects are parented to QQmlEngine (owned by it).
//         init() constructs everything in dependency order and calls
//         registerContextProperties().
//         shutdown() tears down in reverse order.
// ============================================================

#include <QQmlEngine>
#include <memory>

// ── Layer 1 forward declarations (pure C++, no Qt headers) ───────────────────
class ThreadPool;
class NetworkService;
class CameraStore;
class DeviceStore;
class ServerStatusStore;
class AlarmDispatcher;

// ── Layer 2 forward declarations (QObject adapters) ──────────────────────────
class NetworkBridge;
class LoginController;
class SignupController;
class CameraModel;
class DeviceModel;
class ServerStatusModel;
class UserModel;
class VideoManager;
class AlarmManager;

class Core {
public:
  Core();
  ~Core();

  // Called once from App.cpp after Qt engine setup.
  // Constructs all objects, wires signals, registers context properties.
  void init(QQmlEngine &engine);

  // Called from QGuiApplication::aboutToQuit.
  // Tears down in reverse construction order.
  void shutdown();

private:
  void constructLayer1();
  void constructLayer2(QQmlEngine &engine);
  void wireSignals();
  void registerContextProperties(QQmlEngine &engine);

  // ── Layer 1 — owned here (destroyed in ~Core) ────────────────────────
  std::unique_ptr<ThreadPool> threadPool_;
  std::unique_ptr<NetworkService> networkService_;
  std::unique_ptr<CameraStore> cameraStore_;
  std::unique_ptr<DeviceStore> deviceStore_;
  std::unique_ptr<ServerStatusStore> serverStatusStore_;
  std::unique_ptr<AlarmDispatcher> alarmDispatcher_;

  // ── Layer 2 — parented to QQmlEngine, not owned by Core ─────────────
  // Raw pointers only — QQmlEngine destructor handles deletion.
  NetworkBridge *networkBridge_ = nullptr;
  LoginController *login_ = nullptr;
  SignupController *signup_ = nullptr;
  CameraModel *cameraModel_ = nullptr;
  DeviceModel *deviceModel_ = nullptr;
  ServerStatusModel *serverStatus_ = nullptr;
  UserModel *userModel_ = nullptr;
  VideoManager *videoManager_ = nullptr;
  AlarmManager *alarmManager_ = nullptr;
};
