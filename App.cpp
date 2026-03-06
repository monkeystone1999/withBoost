#include "Qt/Back/AlarmManager.hpp"
#include "Qt/Back/CameraModel.hpp"
#include "Qt/Back/DeviceModel.hpp"
#include "Qt/Back/Login.hpp"
#include "Qt/Back/NetworkBridge.hpp"
#include "Qt/Back/ServerStatusModel.hpp"
#include "Qt/Back/Signup.hpp"
#include "Qt/Back/VideoManager.hpp"
#include "Qt/Back/VideoSurfaceItem.hpp"
#include "Src/Network/Session.hpp"
#include <QGuiApplication>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QWKQuick/qwkquickglobal.h>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(AnoMap_frontPlugin)

class PathCaseInterceptor : public QQmlAbstractUrlInterceptor {
public:
  QUrl intercept(const QUrl &path, DataType) override {
    QString urlStr = path.toString();
    if (urlStr.contains("/views/"))
      urlStr.replace("/views/", "/View/");
    return QUrl(urlStr);
  }
};

int main(int argc, char **argv) {
  QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  PathCaseInterceptor interceptor;
  engine.setUrlInterceptor(&interceptor);
  QWK::registerTypes(&engine);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  qmlRegisterType<VideoSurfaceItem>("AnoMap.back", 1, 0, "VideoSurfaceItem");

  // ── Back 객체 생성 ────────────────────────────────────────────────────────
  NetworkBridge *networkBridge = new NetworkBridge(&engine);
  engine.rootContext()->setContextProperty("networkBridge", networkBridge);

  Login *loginController = new Login(networkBridge, &engine);
  engine.rootContext()->setContextProperty("loginController", loginController);

  Signup *signupController = new Signup(networkBridge, &engine);
  engine.rootContext()->setContextProperty("signupController",
                                           signupController);

  CameraModel *cameraModel = new CameraModel(&engine);
  engine.rootContext()->setContextProperty("cameraModel", cameraModel);
  QObject::connect(networkBridge, &NetworkBridge::cameraListReceived,
                   cameraModel, &CameraModel::refreshFromJson);

  DeviceModel *deviceModel = new DeviceModel(&engine);
  engine.rootContext()->setContextProperty("deviceModel", deviceModel);
  QObject::connect(networkBridge, &NetworkBridge::cameraListReceived,
                   deviceModel, &DeviceModel::refreshFromJson);

  ServerStatusModel *serverStatus = new ServerStatusModel(&engine);
  engine.rootContext()->setContextProperty("serverStatus", serverStatus);
  QObject::connect(networkBridge, &NetworkBridge::deviceListReceived,
                   serverStatus, &ServerStatusModel::refreshFromJson);

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
}
