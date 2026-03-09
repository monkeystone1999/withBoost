<<<<<<< Updated upstream
#include "Qt/Back/AlarmManager.hpp"
#include "Qt/Back/CameraModel.hpp"
#include "Qt/Back/DeviceModel.hpp"
#include "Qt/Back/Login.hpp"
#include "Qt/Back/NetworkBridge.hpp"
#include "Qt/Back/Signup.hpp"
#include "Qt/Back/VideoManager.hpp"
=======
// ============================================================
//  App.cpp — Qt engine configuration only
//
//  RULE: This file must not construct any business objects,
//        parse any data, or wire any signals.
//        All of that belongs in Core::init().
//
//  This file is responsible for:
//    1. Qt / rendering API selection
//    2. QQmlApplicationEngine setup (URL interceptor, QWK, import paths)
//    3. Instantiating Core and delegating init / shutdown
//    4. Loading the QML root module
// ============================================================

#include "Core.hpp"
>>>>>>> Stashed changes
#include "Qt/Back/VideoSurfaceItem.hpp"
#include <QGuiApplication>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QWKQuick/qwkquickglobal.h>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(AnoMap_frontPlugin)

// Fixes a case-sensitivity issue in QML import paths on case-sensitive
// file systems that may have been generated with lowercase "views/".
class PathCaseInterceptor : public QQmlAbstractUrlInterceptor {
public:
    QUrl intercept(const QUrl &path, DataType) override {
        QString s = path.toString();
        if (s.contains("/views/"))
            s.replace("/views/", "/View/");
        return QUrl(s);
    }
};

int main(int argc, char **argv) {
    // ── 1. Rendering API — must be set before QGuiApplication ────────────
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // ── 2. Engine setup ───────────────────────────────────────────────────
    PathCaseInterceptor interceptor;
    engine.setUrlInterceptor(&interceptor);
    QWK::registerTypes(&engine);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    // VideoSurfaceItem is a QQuickItem — must be registered before QML load
    qmlRegisterType<VideoSurfaceItem>("AnoMap.back", 1, 0, "VideoSurfaceItem");

    // ── 3. Core: constructs everything, wires signals, registers QML props ─
    Core core;
    core.init(engine);

    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&] {
        core.shutdown();
    });

    // ── 4. QML import paths + load ────────────────────────────────────────
    engine.addImportPath("qrc:/qt/qml");
    engine.addImportPath(":/qt/qml");
    engine.addImportPath("qrc:/");
    engine.loadFromModule("AnoMap.front", "Main");

<<<<<<< Updated upstream
  DeviceModel *deviceModel = new DeviceModel(&engine);
  engine.rootContext()->setContextProperty("deviceModel", deviceModel);
  QObject::connect(networkBridge, &NetworkBridge::deviceListReceived,
                   deviceModel, &DeviceModel::refreshFromJson);

  qDebug() << "[App] DeviceModel registered";

  VideoManager *videoManager = new VideoManager(&engine);
  engine.rootContext()->setContextProperty("videoManager", videoManager);
  QObject::connect(cameraModel, &CameraModel::urlsUpdated, videoManager,
                   &VideoManager::registerUrls);

  AlarmManager *alarmManager = new AlarmManager(&engine);
  engine.rootContext()->setContextProperty("alarmManager", alarmManager);
  QObject::connect(networkBridge, &NetworkBridge::aiResultReceived,
                   alarmManager, &AlarmManager::parseAiMessage);

  // ── 핵심: 앱 종료 시 Back 을 순서대로 정리 ───────────────────────────────
  // QGuiApplication::aboutToQuit 은 이벤트 루프가 종료 직전에 발생한다.
  // 이 시점에 Back 리소스를 먼저 정리해야
  //   1) FFmpeg 스레드 (Video::m_decodeThread) 가 join 되고
  //   2) boost::asio io_thread_ 가 join 되어
  // 프로세스가 깨끗하게 종료된다.
  QObject::connect(&app, &QGuiApplication::aboutToQuit, [&]() {
    // 1. 모든 FFmpeg 스트리밍 스레드 종료 (Video::stopStream → join)
    videoManager->clearAll();

    // 2. 모든 TCP 소켓 닫기 → io_context 에 남은 async 작업 취소
    //    → work_guard_.reset() 후 io_thread_.join() 이 정상 완료됨
    networkBridge->disconnectAll();
  });

  engine.addImportPath("qrc:/qt/qml");
  engine.addImportPath(":/qt/qml");
  engine.addImportPath("qrc:/");

  engine.loadFromModule("AnoMap.front", "Main");

  return app.exec();
  // app.exec() 반환 → aboutToQuit 시그널 발생 → 위 람다 실행
  // → videoManager, networkBridge 정리 완료
  // → engine 소멸 → QObject 자식들 소멸
  // → 프로세스 정상 종료
=======
    return app.exec();
>>>>>>> Stashed changes
}
