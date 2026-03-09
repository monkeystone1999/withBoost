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

#include <memory>
#include <QQmlEngine>

// ── Layer 1 forward declarations (pure C++, no Qt headers) ───────────────────
class ThreadPool;
class NetworkService;
class CameraStore;
class DeviceStore;
class ServerStatusStore;
class AlarmDispatcher;

// ── Layer 2 forward declarations (QObject adapters) ──────────────────────────
class NetworkBridge;
class Login;
class Signup;
class CameraModel;
class DeviceModel;
class ServerStatusModel;
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
    std::unique_ptr<ThreadPool>         m_threadPool;
    std::unique_ptr<NetworkService>     m_networkService;
    std::unique_ptr<CameraStore>        m_cameraStore;
    std::unique_ptr<DeviceStore>        m_deviceStore;
    std::unique_ptr<ServerStatusStore>  m_serverStatusStore;
    std::unique_ptr<AlarmDispatcher>    m_alarmDispatcher;

    // ── Layer 2 — parented to QQmlEngine, not owned by Core ─────────────
    // Raw pointers only — QQmlEngine destructor handles deletion.
    NetworkBridge     *m_networkBridge   = nullptr;
    Login             *m_login           = nullptr;
    Signup            *m_signup          = nullptr;
    CameraModel       *m_cameraModel     = nullptr;
    DeviceModel       *m_deviceModel     = nullptr;
    ServerStatusModel *m_serverStatus    = nullptr;
    VideoManager      *m_videoManager    = nullptr;
    AlarmManager      *m_alarmManager    = nullptr;
};
