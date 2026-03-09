# AnoMap — Architecture & Refactoring Guide

> **Audience:** Every developer who touches this codebase.  
> **Purpose:** Document the _When / Where / Why / How_ of every architectural
> decision and every required code change so that the project runs correctly,
> is maintainable, and can grow without accumulating technical debt.

---

## Table of Contents

1. [Mental Model — Three Layers](#1-mental-model--three-layers)
2. [Dependency Graph (current target)](#2-dependency-graph-current-target)
3. [Startup Sequence](#3-startup-sequence)
4. [Per-Component Analysis](#4-per-component-analysis)
   - 4.1 [Config.hpp](#41-confighpp)
   - 4.2 [ThreadPool](#42-threadpool)
   - 4.3 [INetworkService / NetworkService](#43-inetworkservice--networkservice)
   - 4.4 [Domain Stores (CameraStore, DeviceStore, ServerStatusStore, AlarmDispatcher)](#44-domain-stores)
   - 4.5 [Core.cpp — Missing Orchestrator](#45-corecpp--missing-orchestrator)
   - 4.6 [NetworkBridge (QObject)](#46-networkbridge-qobject)
   - 4.7 [Login / Signup (QObject)](#47-login--signup-qobject)
   - 4.8 [CameraModel / DeviceModel / ServerStatusModel (QObject)](#48-models-qobject)
   - 4.9 [VideoWorker / VideoManager (QObject)](#49-videoworker--videomanager-qobject)
   - 4.10 [AlarmManager (QObject)](#410-alarmmanager-qobject)
   - 4.11 [App.cpp](#411-appcpp)
5. [What Must Be Created / Changed / Deleted](#5-what-must-be-created--changed--deleted)
6. [Threading Rules](#6-threading-rules)
7. [Packet Reference](#7-packet-reference)
8. [Glossary](#8-glossary)

---

## 1. Mental Model — Three Layers

```
┌─────────────────────────────────────────────────────────────────┐
│  LAYER 3 — QML / UI                                             │
│  .qml files only. Reads model roles, calls Q_INVOKABLE methods. │
│  Zero business logic. Zero JSON. Zero threading.                │
└────────────────────────────┬────────────────────────────────────┘
                             │  context properties  (read-only views)
                             │  Q_INVOKABLE commands (write path)
┌────────────────────────────▼────────────────────────────────────┐
│  LAYER 2 — Qt Adapters  (Qt/Back/)                              │
│  QObject subclasses.                                            │
│  Single responsibility: convert  Qt types ↔ std types.         │
│  Must NOT: own infrastructure, parse JSON, run algorithms.      │
│  May:      emit Qt signals, hold Q_PROPERTY state,              │
│            forward calls to Layer 1 via injected pointers.      │
└────────────────────────────┬────────────────────────────────────┘
                             │  std::string / plain structs
                             │  ThreadPool tasks / std::function callbacks
┌────────────────────────────▼────────────────────────────────────┐
│  LAYER 1 — Pure C++  (Src/)                                     │
│  No Qt. No GUI. Freely testable with gtest / catch2.            │
│  Owns: ThreadPool, NetworkService, Domain Stores.               │
│  Config.hpp — single source of truth for all constants.         │
└─────────────────────────────────────────────────────────────────┘
```

**Golden Rule:** Data always flows  
`Network (asio thread) → ThreadPool (parse) → Qt signal (GUI thread) → QML`.  
The GUI thread never touches JSON or performs blocking I/O.

---

## 2. Dependency Graph (current target)

```
App.cpp
 └─ Core::init()                          ← THE MISSING PIECE (§4.5)
     ├─ ThreadPool            (Src)
     ├─ NetworkService        (Src)  ←─ INetworkService interface
     ├─ CameraStore           (Src)
     ├─ DeviceStore           (Src)
     ├─ ServerStatusStore     (Src)
     ├─ AlarmDispatcher       (Src)
     │
     ├─ NetworkBridge(service)           (Qt/Back)  ← type bridge
     ├─ Login(bridge)                    (Qt/Back)
     ├─ Signup(bridge)                   (Qt/Back)
     ├─ CameraModel                      (Qt/Back)
     ├─ DeviceModel                      (Qt/Back)
     ├─ ServerStatusModel                (Qt/Back)
     ├─ VideoManager                     (Qt/Back)
     └─ AlarmManager(dispatcher)         (Qt/Back)
```

Everything in `Src/` is constructed **before** any QObject.  
QObjects receive injected pointers; they never `new` their own infrastructure.

---

## 3. Startup Sequence

```
main()
  │
  ├─ QGuiApplication + QQmlApplicationEngine   [Qt setup]
  │
  ├─ Core::init(engine)                         [REQUIRED — see §4.5]
  │    ├─ 1. construct ThreadPool(4)
  │    ├─ 2. construct NetworkService
  │    ├─ 3. construct CameraStore / DeviceStore / ServerStatusStore
  │    ├─ 4. construct AlarmDispatcher(pool)
  │    │
  │    ├─ 5. construct NetworkBridge(&service)
  │    ├─ 6. construct Login(bridge) / Signup(bridge)
  │    ├─ 7. construct CameraModel / DeviceModel / ServerStatusModel
  │    ├─ 8. construct VideoManager
  │    ├─ 9. construct AlarmManager(&dispatcher)
  │    │
  │    ├─ 10. wire signals (NetworkBridge signals → Core dispatch lambdas
  │    │       → ThreadPool tasks → Qt model slots)
  │    │
  │    └─ 11. register context properties in QQmlContext
  │
  ├─ engine.loadFromModule("AnoMap.front", "Main")
  │
  └─ app.exec()
        │  [user presses Login]
        │
        ├─ Login::login()  → NetworkBridge::connectToServer()
        │                  → NetworkService::connect()  [asio thread starts]
        │
        │  [server sends SUCCESS(0x02)]
        │  → NetworkCallbacks::onLoginResult  [asio strand]
        │  → NetworkBridge emits loginSuccess  [GUI thread via invokeMethod]
        │  → Login::handleLoginSuccess()
        │
        │  [server sends CAMERA(0x07)  — on connect + every 5 s]
        │  → NetworkCallbacks::onCameraList  [asio strand]
        │  → NetworkBridge emits cameraListReceived  [GUI thread]
        │  → Core lambda: pool.submit( CameraStore::updateFromJson )
        │                 pool.submit( DeviceStore::updateFromJson )
        │  → callbacks post to GUI thread:
        │       CameraModel::onStoreUpdated(snapshot)
        │       DeviceModel::onStoreUpdated(snapshot)
        │
        │  [server sends DEVICE(0x04)  — every 5 s]
        │  → NetworkCallbacks::onDeviceStatus  [asio strand]
        │  → NetworkBridge emits deviceStatusReceived  [GUI thread]
        │  → Core lambda: pool.submit( ServerStatusStore::updateFromJson )
        │  → callback posts to GUI: ServerStatusModel::onStoreUpdated(data)
        │
        │  [server sends AI(0x06)]
        │  → AlarmDispatcher::dispatch() → pool task → AlarmManager::onAlarm()
        │
        └─ aboutToQuit:  VideoManager::clearAll()
                         NetworkService::disconnect()
```

---

## 4. Per-Component Analysis

---

### 4.1 Config.hpp

**File:** `Src/Config.hpp`

| | |
|---|---|
| **When** | Read at compile time. No runtime cost. |
| **Where** | `Src/Config.hpp` only — never duplicated. |
| **Why** | `Login.cpp` hard-coded `"192.168.0.58"` and `"20000"`. `Signup.cpp` hard-coded `"192.168.0.52"` (different IP — likely a bug). `App.cpp` hard-coded thread count comments. One file change deploys to all environments. |
| **How** | `constexpr std::string_view` constants. Include only in `Core.cpp` and `NetworkService.cpp`. QObject adapters must never include Config.hpp directly — they receive values via constructor injection. |

**Required changes:**

1. **`Signup.cpp`** — `connectToServer("Main", "192.168.0.52", "20000")` must use `Config::SERVER_HOST` / `Config::SERVER_PORT`. The `.52` vs `.58` discrepancy is a latent bug that silently breaks signup in production.
2. **`Login.cpp`** — same hard-coded strings must be removed (already partially done in newer NetworkBridge but Login still calls old `connectToServer("Main", ...)` overload).

---

### 4.2 ThreadPool

**File:** `Src/Thread/ThreadPool.hpp/.cpp`

| | |
|---|---|
| **When** | Constructed once in `Core::init()`, before any network connection. Lives until `Core` destructor. |
| **Where** | `Src/Thread/` — zero Qt dependency. |
| **Why** | All JSON parsing (camera list = 13 objects × every 5 s, device status, AI packets) currently happens on the GUI thread inside QObject slots. A single 13-camera CAMERA packet parse takes 1–5 ms on debug builds — enough to cause visible frame drops. ThreadPool offloads all parsing. |
| **How** | `ThreadPool(Config::THREAD_POOL_SIZE)` = 4 threads. Task distribution: thread 1–2 camera/device parse, thread 3 alarm parse, thread 4 spare / burst. All tasks are fire-and-forget lambdas that capture `std::string` by value (no shared mutable state inside task). |

**Current status:** ✅ Implemented. `AlarmDispatcher` already uses it.  
**Missing:** `CameraStore::updateFromJson` and `DeviceStore::updateFromJson` are called **directly** in the signal lambda (GUI thread) — they must be wrapped in `pool.submit(...)`. See §4.5.

---

### 4.3 INetworkService / NetworkService

**Files:** `Src/Network/INetworkService.hpp`, `Src/Network/NetworkService.hpp/.cpp`

| | |
|---|---|
| **When** | `NetworkService` constructed in `Core::init()` step 2. `connect()` called by `NetworkBridge::connectToServer()` which is called from `Login::login()`. |
| **Where** | `Src/Network/` — pure C++, owns `SessionManager`. |
| **Why** | Previously `NetworkBridge` (a QObject) owned `SessionManager` directly. That made the network layer untestable and created a layering violation: a UI adapter managing I/O infrastructure. |
| **How** | `INetworkService` is the seam. `NetworkService` wraps `SessionManager`. `NetworkBridge` holds `INetworkService*` (non-owning). For tests, a `MockNetworkService` can be injected. `send()` accepts a raw `uint8_t` message type to avoid exposing `MessageType` enum to the Qt layer. |

**Current status:** ✅ Implemented.  
**Known issue:** `NetworkBridge` still exposes `hasSession(QString)` and `disconnectFromServer(QString)` in some older call sites (`Login.cpp` calls `m_bridge->hasSession("Main")` and `m_bridge->disconnectFromServer("Main")`). These must be replaced with `m_bridge->isConnected()` and `m_bridge->disconnect()`.

---

### 4.4 Domain Stores

**Files:** `Src/Domain/CameraStore`, `DeviceStore`, `ServerStatusStore`, `AlarmDispatcher`

All four follow the same pattern:

```
ThreadPool thread:
  Store::updateFromJson(rawJson, callback)
    │
    ├─ parse with nlohmann::json  (no Qt, no lock on parse)
    ├─ lock_guard → update m_data
    └─ invoke callback(parsedSnapshot)
           │
           └─ Core.cpp posts snapshot to GUI thread via invokeMethod
                  └─ QAbstractListModel::beginResetModel / dataChanged
```

| Store | Packet | Filter | Output type |
|---|---|---|---|
| `CameraStore` | CAMERA (0x07) | all entries | `vector<CameraData>` |
| `DeviceStore` | CAMERA (0x07) | `type=="SUB_PI"` only | `vector<DeviceData>` |
| `ServerStatusStore` | DEVICE (0x04) | none | `ServerStatusData` |
| `AlarmDispatcher` | AI (0x06) | `type=="alarm"` | `AlarmEvent` |

**Current status:** ✅ All four `.cpp` files implemented.  
**Missing:** `Core.cpp` is not wiring `pool.submit()` around `updateFromJson` calls. The stores exist but are never called. See §4.5.

---

### 4.5 Core.cpp — Missing Orchestrator

**File:** `Core.cpp` (PROJECT ROOT — **does not exist yet**)

This is the **most critical missing file** in the project.

| | |
|---|---|
| **When** | Called from `main()` / `App.cpp` as the very first step after Qt application setup. Replaces the 60-line construction block in `App.cpp`. |
| **Where** | Project root, alongside `App.cpp`. |
| **Why** | `App.cpp` currently constructs all objects AND wires all signals. This violates Single Responsibility. `App.cpp` should only configure the Qt engine (graphics API, URL interceptor, QML import paths). The object graph and its wiring belong in `Core`. Without `Core`, the `ThreadPool` and all Domain Stores are never instantiated — they are dead code. |
| **How** | `Core` owns all Layer 1 objects via `std::unique_ptr`. It exposes Layer 2 QObjects as raw pointers (owned by `QQmlEngine` as children). Its `init(QQmlEngine&)` method: constructs Layer 1 → constructs Layer 2 (injecting Layer 1 pointers) → wires signals → registers context properties. Its `shutdown()` method tears down in reverse order. |

**Skeleton:**

```cpp
// Core.hpp
#pragma once
#include <memory>
#include <QQmlEngine>

// Forward declarations — no Qt headers in Src/
class ThreadPool;
class NetworkService;
class CameraStore;
class DeviceStore;
class ServerStatusStore;
class AlarmDispatcher;

// Qt/Back forward declarations
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

    // Called once from App.cpp after engine setup.
    // Registers all context properties.
    void init(QQmlEngine &engine);

    // Called from aboutToQuit.
    void shutdown();

private:
    void wireSignals();
    void registerContextProperties(QQmlEngine &engine);

    // Layer 1 — pure C++ (owned here)
    std::unique_ptr<ThreadPool>         m_threadPool;
    std::unique_ptr<NetworkService>     m_networkService;
    std::unique_ptr<CameraStore>        m_cameraStore;
    std::unique_ptr<DeviceStore>        m_deviceStore;
    std::unique_ptr<ServerStatusStore>  m_serverStatusStore;
    std::unique_ptr<AlarmDispatcher>    m_alarmDispatcher;

    // Layer 2 — Qt adapters (owned by QQmlEngine as children)
    NetworkBridge    *m_networkBridge    = nullptr;
    Login            *m_login            = nullptr;
    Signup           *m_signup           = nullptr;
    CameraModel      *m_cameraModel      = nullptr;
    DeviceModel      *m_deviceModel      = nullptr;
    ServerStatusModel*m_serverStatus     = nullptr;
    VideoManager     *m_videoManager     = nullptr;
    AlarmManager     *m_alarmManager     = nullptr;
};
```

**`Core::wireSignals()` — the critical wiring (pseudo-code):**

```cpp
void Core::wireSignals() {
    // ── CAMERA packet → ThreadPool parse → GUI model update ──────────────
    QObject::connect(m_networkBridge, &NetworkBridge::cameraListReceived,
        [this](const QString &json) {
            const std::string s = json.toStdString();

            // CameraStore: parse on ThreadPool, update CameraModel on GUI
            m_threadPool->submit([this, s] {
                m_cameraStore->updateFromJson(s, [this](auto snapshot) {
                    QMetaObject::invokeMethod(m_cameraModel,
                        [this, snap = std::move(snapshot)]() mutable {
                            m_cameraModel->onStoreUpdated(std::move(snap));
                        }, Qt::QueuedConnection);
                });
            });

            // DeviceStore: same pattern, different model
            m_threadPool->submit([this, s] {
                m_deviceStore->updateFromJson(s, [this](auto snapshot) {
                    QMetaObject::invokeMethod(m_deviceModel,
                        [this, snap = std::move(snapshot)]() mutable {
                            m_deviceModel->onStoreUpdated(std::move(snap));
                        }, Qt::QueuedConnection);
                });
            });
        });

    // ── DEVICE packet → ThreadPool parse → GUI model update ──────────────
    QObject::connect(m_networkBridge, &NetworkBridge::deviceStatusReceived,
        [this](const QString &json) {
            const std::string s = json.toStdString();
            m_threadPool->submit([this, s] {
                m_serverStatusStore->updateFromJson(s, [this](auto data) {
                    QMetaObject::invokeMethod(m_serverStatus,
                        [this, d = std::move(data)]() mutable {
                            m_serverStatus->onStoreUpdated(std::move(d));
                        }, Qt::QueuedConnection);
                });
            });
        });

    // ── AI packet → AlarmDispatcher (already uses ThreadPool) ─────────────
    QObject::connect(m_networkBridge, &NetworkBridge::aiResultReceived,
        [this](const QString &json) {
            m_alarmDispatcher->dispatch(json.toStdString(),
                [this](AlarmEvent ev) {
                    QMetaObject::invokeMethod(m_alarmManager,
                        [this, ev]() { m_alarmManager->onAlarm(ev); },
                        Qt::QueuedConnection);
                });
        });

    // ── VideoManager: URL list from CameraModel ───────────────────────────
    QObject::connect(m_cameraModel, &CameraModel::urlsUpdated,
                     m_videoManager, &VideoManager::registerUrls);
}
```

---

### 4.6 NetworkBridge (QObject)

**File:** `Qt/Back/NetworkBridge.hpp/.cpp`

| | |
|---|---|
| **When** | Constructed in `Core::init()` step 5, after `NetworkService` is ready. |
| **Where** | `Qt/Back/` — only Qt-related type conversions here. |
| **Why** | QML needs Q_INVOKABLE methods. The bridge translates `QString` parameters to `std::string` before calling `INetworkService::send()`. |
| **How** | Constructor takes `INetworkService*`. `wireCallbacks()` was originally an explicit step; in the new design `connectToServer()` does this inline (current implementation). This is acceptable — keep it as-is. |

**Current issues to fix:**

1. `connectToServer(const QString &name, ...)` — the old signature accepted a session `name`. The new `INetworkService` interface does not expose session names. **Remove the `name` parameter.** QML callers use `networkBridge.connectToServer(host, port)`.
2. `hasSession(QString)` — remove. Replace call sites with `isConnected()`.
3. `disconnectFromServer(QString)` — remove. Replace with `disconnect()`.
4. `sendListPending()` / `sendApprove()` — these are admin-only ASSIGN operations. They are correctly implemented.

---

### 4.7 Login / Signup (QObject)

**Files:** `Qt/Back/Login.hpp/.cpp`, `Qt/Back/Signup.hpp/.cpp`

| | |
|---|---|
| **When** | Constructed in `Core::init()` step 6. |
| **Where** | `Qt/Back/` |
| **Why** | QML login form needs property bindings (`isLoading`, `isError`, `errorMessage`). The QObject wraps the async login state machine. |
| **How** | `Login` listens to `NetworkBridge::loginSuccess/loginFailed` and drives `NetworkBridge::connectToServer` → `sendLogin`. |

**Current issues to fix:**

1. **`Login.cpp` line `m_bridge->hasSession("Main")`** — `hasSession(QString)` no longer exists on the new `NetworkBridge`. Replace with `m_bridge->isConnected()`.
2. **`Login.cpp` line `m_bridge->disconnectFromServer("Main")`** — replace with `m_bridge->disconnect()`.
3. **`Login.cpp` line `m_bridge->connectToServer("Main", "192.168.0.58", "20000")`** — the new `connectToServer` takes only `(host, port)`. The server address must come from `Core` passing `Config::SERVER_HOST` and `Config::SERVER_PORT` into Login's constructor, or Login calls `m_bridge->connectToServer()` with no arguments (Core pre-configures the bridge). **Recommended:** Core injects host/port strings into Login constructor; Login calls `m_bridge->connectToServer(m_host, m_port)`.
4. **`Signup.cpp` line `"192.168.0.52"`** — this is almost certainly a copy-paste error (`Config::SERVER_HOST` = `.58`). Fix to use injected host.
5. **`Signup.cpp`** has the same `hasSession` / old `connectToServer` issues as Login.

---

### 4.8 Models (QObject)

**Files:** `CameraModel`, `DeviceModel`, `ServerStatusModel`

| | |
|---|---|
| **When** | Constructed in `Core::init()` step 7. Updated on GUI thread via `onStoreUpdated()`. |
| **Where** | `Qt/Back/` |
| **Why** | `QAbstractListModel` must live on the GUI thread. It is a pure view of the Domain Store snapshot. |
| **How** | Currently `refreshFromJson(QString)` does all parsing on the GUI thread. After the refactor, the models get a new slot `onStoreUpdated(vector<CameraData>)` that simply calls `beginResetModel()`, replaces the vector, and calls `endResetModel()`. No JSON. No `QString`. The slot always runs on the GUI thread because `Core::wireSignals()` posts via `Qt::QueuedConnection`. |

**Required new slots to add to each model:**

```cpp
// CameraModel
public slots:
    void onStoreUpdated(std::vector<CameraData> snapshot);
    // Keep refreshFromJson(QString) ONLY as a legacy fallback during transition

// DeviceModel
public slots:
    void onStoreUpdated(std::vector<DeviceData> snapshot);

// ServerStatusModel
public slots:
    void onStoreUpdated(ServerStatusData data);
```

**Why keep `refreshFromJson` temporarily?**  
During transition, if `Core.cpp` is not yet wired, the old path `signal → refreshFromJson` keeps the UI alive. Remove it only after `Core::wireSignals()` is confirmed working.

**`DeviceModel` invokable query methods** (`hasDevice`, `hasMotor`, `deviceIp`) currently parse `QString` and search `QList`. After the refactor, they should delegate to `DeviceStore` (injected pointer) for thread-safe answers:

```cpp
// DeviceModel.hpp — inject DeviceStore
explicit DeviceModel(DeviceStore *store, QObject *parent = nullptr);

Q_INVOKABLE bool    hasDevice(const QString &url) const;  // delegates to m_store->hasDevice(url.toStdString())
Q_INVOKABLE QString deviceIp (const QString &url) const;  // delegates to m_store->deviceIp(url.toStdString())
```

This removes the `QHash<QString, int> m_byUrl` lookup table from `DeviceModel` entirely — `DeviceStore` is the single source of truth.

---

### 4.9 VideoWorker / VideoManager (QObject)

**Files:** `Qt/Back/VideoWorker.hpp/.cpp`, `VideoManager.hpp/.cpp`

| | |
|---|---|
| **When** | `VideoManager::registerUrls()` is called by `CameraModel::urlsUpdated` signal every time the camera list changes. |
| **Where** | `Qt/Back/` |
| **Why** | FFmpeg decode threads must be persistent across UI rebuilds. `VideoManager` ensures one `VideoWorker` (= one FFmpeg thread) per URL, never duplicated. |
| **How** | `VideoWorker` uses `std::atomic<shared_ptr<vector<uint8_t>>>` for lock-free frame hand-off from the FFmpeg thread to the render thread. `frameReady()` signal goes via `Qt::QueuedConnection` with coalescing (`m_signalPending` flag). |

**Current status:** ✅ Design is correct and stable.

**One improvement needed:** `VideoManager` is currently always constructed even for users who might not have video access. `Core::init()` should construct `VideoManager` and only call `registerUrls` after successful login. Currently `CameraModel::urlsUpdated` fires on first `refreshFromJson`, which already implies post-login. This is fine as-is.

**Thread safety note:** `VideoWorker` lives on the GUI thread (QObject parent = VideoManager). Its `m_video` (FFmpeg) runs on a `std::thread` inside `Video.cpp`. The atomic buffer ensures correctness. Do NOT add `QMutex` to `VideoWorker` — it would serialize the render thread with the FFmpeg thread and cause the exact stutter the atomic was designed to eliminate.

---

### 4.10 AlarmManager (QObject)

**File:** `Qt/Back/AlarmManager.hpp/.cpp`

| | |
|---|---|
| **When** | Receives parsed `AlarmEvent` from `AlarmDispatcher` callback, posted to GUI thread by Core. |
| **Where** | `Qt/Back/` |
| **Why** | QML alarm panel needs `alarmTriggered(title, detail, severity)` signal. |
| **How** | Currently `parseAiMessage(QString)` does JSON parsing on the GUI thread. After refactor, it receives a pre-parsed `AlarmEvent` struct. |

**Required changes:**

1. Add `void onAlarm(AlarmEvent ev)` slot (called from Core's `QMetaObject::invokeMethod` after ThreadPool parse).
2. Rename `parseAiMessage` to `onAiJson` and keep it as a **direct-connect** legacy slot that immediately calls `m_dispatcher->dispatch(...)`. This way the JSON never blocks the GUI — the ThreadPool does the work.

**New AlarmManager.hpp:**
```cpp
#include "Src/Domain/AlarmDispatcher.hpp"
#include "Src/Domain/AlarmEvent.hpp"

class AlarmManager : public QObject {
    Q_OBJECT
public:
    explicit AlarmManager(AlarmDispatcher *dispatcher, QObject *parent = nullptr);

public slots:
    // Called by Core on GUI thread with pre-parsed event
    void onAlarm(AlarmEvent ev);

    // Legacy: direct connect from NetworkBridge::aiResultReceived
    // Immediately submits parse task to ThreadPool via dispatcher
    void onAiJson(const QString &json);

signals:
    void alarmTriggered(QString title, QString detail, int severity);

private:
    AlarmDispatcher *m_dispatcher;  // non-owning
};
```

---

### 4.11 App.cpp

**File:** `App.cpp` (project root)

| | |
|---|---|
| **When** | `main()` entry point. |
| **Where** | Project root. |
| **Why** | Currently does everything: Qt setup, object construction, signal wiring, context registration. After refactor it does only Qt-engine-level work. |
| **How** | Delegate all construction to `Core::init()`. Keep only: graphics API selection, URL interceptor, QWK registration, QML import paths, `loadFromModule`, `aboutToQuit` hook. |

**Target App.cpp:**
```cpp
int main(int argc, char **argv) {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    PathCaseInterceptor interceptor;
    engine.setUrlInterceptor(&interceptor);
    QWK::registerTypes(&engine);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []{ QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    qmlRegisterType<VideoSurfaceItem>("AnoMap.back", 1, 0, "VideoSurfaceItem");

    Core core;
    core.init(engine);   // constructs everything, registers context properties

    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&] {
        core.shutdown();
    });

    engine.addImportPath("qrc:/qt/qml");
    engine.addImportPath(":/qt/qml");
    engine.addImportPath("qrc:/");
    engine.loadFromModule("AnoMap.front", "Main");

    return app.exec();
}
```

---

## 5. What Must Be Created / Changed / Deleted

### 5.1 Files to CREATE

| File | Layer | Priority | Description |
|------|-------|----------|-------------|
| `Core.hpp` | Root | 🔴 Critical | Core class declaration |
| `Core.cpp` | Root | 🔴 Critical | Construction + wiring orchestrator |

### 5.2 Files to CHANGE

| File | Change | Priority |
|------|--------|----------|
| `App.cpp` | Replace 60-line construction block with `Core core; core.init(engine);` | 🔴 Critical |
| `Qt/Back/NetworkBridge.hpp/.cpp` | Remove `hasSession`, `disconnectFromServer` (old name), `connectToServer(name,host,port)` → `connectToServer(host,port)` | 🔴 Critical |
| `Qt/Back/Login.cpp` | Replace `hasSession("Main")` → `isConnected()`, `disconnectFromServer("Main")` → `disconnect()`, `connectToServer("Main",...)` → `connectToServer(m_host, m_port)` | 🔴 Critical |
| `Qt/Back/Signup.cpp` | Same as Login.cpp + fix `"192.168.0.52"` → injected host | 🔴 Critical |
| `Qt/Back/CameraModel.hpp/.cpp` | Add `onStoreUpdated(vector<CameraData>)` slot | 🟡 High |
| `Qt/Back/DeviceModel.hpp/.cpp` | Add `onStoreUpdated(vector<DeviceData>)` slot; inject `DeviceStore*`; remove `m_byUrl` QHash lookup | 🟡 High |
| `Qt/Back/ServerStatusModel.hpp/.cpp` | Add `onStoreUpdated(ServerStatusData)` slot; remove all JSON parsing | 🟡 High |
| `Qt/Back/AlarmManager.hpp/.cpp` | Inject `AlarmDispatcher*`; add `onAlarm(AlarmEvent)` slot; rename `parseAiMessage` → `onAiJson` | 🟡 High |
| `Qt/Back/CMakeLists.txt` | No change needed unless `Dashboard.hpp/.cpp` (currently unused) should be removed | 🟢 Low |
| `Src/CMakeLists.txt` | Add `Domain/*.hpp Domain/*.cpp` to `AnoMapSTD` sources | 🔴 Critical |
| `CMakeLists.txt` (root) | Add `Core.cpp` to `AnoMap` executable sources | 🔴 Critical |

### 5.3 Files to DELETE

| File | Reason |
|------|--------|
| `Qt/Back/Dashboard.hpp/.cpp` | Empty / dead code. Not registered in `App.cpp`. Remove to avoid confusion. |

### 5.4 Files that are CORRECT — do not change

| File | Status |
|------|--------|
| `Src/Config.hpp` | ✅ |
| `Src/Thread/ThreadPool.hpp/.cpp` | ✅ |
| `Src/Network/INetworkService.hpp` | ✅ |
| `Src/Network/NetworkService.hpp/.cpp` | ✅ |
| `Src/Network/Session.hpp/.cpp` | ✅ |
| `Src/Domain/CameraStore.hpp/.cpp` | ✅ |
| `Src/Domain/DeviceStore.hpp/.cpp` | ✅ |
| `Src/Domain/ServerStatusStore.hpp/.cpp` | ✅ |
| `Src/Domain/AlarmDispatcher.hpp/.cpp` | ✅ |
| `Qt/Back/VideoWorker.hpp/.cpp` | ✅ |
| `Qt/Back/VideoManager.hpp/.cpp` | ✅ |
| `Qt/Front/**` | ✅ (see separate QML spec) |

---

## 6. Threading Rules

```
Thread              Owns / Runs                          May NOT do
────────────────────────────────────────────────────────────────────────
GUI thread          All QObjects, all Qt signals,        Block > 1 ms.
                    QML engine, model updates.           Parse JSON.
                                                         Acquire mutex
                                                         held by other thread.

boost::asio thread  SessionManager::io_context_,        Touch any QObject.
(io_thread_)        ConnectServer read/write loops.     Emit Qt signals.
                                                         Lock m_mutex of stores
                                                         for > 1 µs.

ThreadPool          Domain store parsing,               Touch any QObject.
threads (×4)        AlarmDispatcher tasks.              Emit Qt signals.
                                                         Call store methods
                                                         with long-held locks.

FFmpeg threads      Video::m_decodeThread per stream.   Touch any QObject.
(one per stream)    Writes to VideoWorker::m_atomicBuf. Acquire non-atomic locks.
                    Emits invokeMethod→GUI.

Render thread       VideoSurfaceItem::updatePaintNode.  Acquire QMutex.
                    Reads VideoWorker::m_atomicBuf.     Touch model data directly.
```

**Crossing the boundary correctly:**

```cpp
// From asio/ThreadPool → GUI thread
QMetaObject::invokeMethod(qobject_ptr,
    [captured_data]() { /* runs on GUI thread */ },
    Qt::QueuedConnection);  // ALWAYS QueuedConnection, never Direct

// From GUI thread → ThreadPool
m_threadPool->submit([data_by_value]() { /* runs on pool thread */ });
// Capture by value only. Never capture QObject* or Qt types.
```

---

## 7. Packet Reference

| Header | Hex | Direction | Frequency | Body format | Handler |
|--------|-----|-----------|-----------|-------------|---------|
| LOGIN | 0x01 | Client→Server | On demand | `{"id":"...","pw":"..."}` | Login::login() |
| SUCCESS | 0x02 | Server→Client | Once on login | `{"state":"admin"\|"user","success":true,"username":"..."}` | NetworkBridge → loginSuccess signal |
| FAIL | 0x03 | Server→Client | On error | `{"error":"...","context":"login"}` | NetworkBridge → loginFailed signal |
| DEVICE | 0x04 | Bidirectional | Client sends on demand; Server sends every 5 s | **Server→Client:** `{"devices":[{cpu,ip,memory,temp,uptime,pending_events}],"server":{...}}` (server key = admin only) **Client→Server:** `{"device":"ip","motor":"w\|a\|s\|d\|auto\|unauto"}` | ServerStatusStore (receive); sendDevice() (send) |
| AVAILABLE | 0x05 | Server→Client | — | TBD | — |
| AI | 0x06 | Server→Client | On event | `{"type":"alarm","title":"...","detail":"...","severity":1-3}` | AlarmDispatcher |
| CAMERA | 0x07 | Server→Client | Once on login + every 5 s | `[{"ip":"...","is_online":bool,"source_url":"rtsp://...","type":"HANWHA"\|"SUB_PI"}]` | CameraStore + DeviceStore |
| ASSIGN | 0x08 | Bidirectional | Admin only | `{"action":"register"\|"list_pending"\|"approve","target_username":"..."}` | Signup / admin panel |

**Important invariants:**
- The CAMERA packet always contains all 13 cameras (12 HANWHA + 1 SUB_PI).
- A standalone DEVICE packet (0x04) from the server is NOT a device list — it is a system status payload. Do not parse it as a device list.
- WASD motor commands must force `motorAuto = false` immediately. The auto/unauto state packet is sent **1 second after the last WASD key**, not immediately.

---

## 8. Glossary

| Term | Meaning |
|------|---------|
| **Layer 1** | Pure C++ code in `Src/`. No Qt. |
| **Layer 2** | Qt adapter QObjects in `Qt/Back/`. Type conversion only. |
| **Layer 3** | QML UI in `Qt/Front/`. |
| **Store** | A Layer 1 class that owns a thread-safe in-memory snapshot of one domain (cameras, devices, server status). It parses JSON on the ThreadPool and notifies via callback. |
| **Bridge** | A Layer 2 QObject (`NetworkBridge`) that converts `QString ↔ std::string` and emits Qt signals. |
| **DI** | Dependency Injection — constructors receive pointers to their dependencies; they do not `new` infrastructure. |
| **Core** | The single orchestrator (`Core.cpp`) that constructs all objects in dependency order and wires all signals. The application has exactly one `Core` instance. |
| **Snapshot** | A by-value copy of a Store's internal vector, safe to move across thread boundaries inside a lambda. |
| **HANWHA** | IP camera type. AI analysis target. 12 cameras in current deployment. |
| **SUB_PI** | Raspberry Pi camera type. Motor-controllable. 1 camera in current deployment (`192.168.0.43`). |
