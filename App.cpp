// ============================================================
//  App.cpp ??Qt engine configuration only
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
#include "Qt/Back/Services/VideoStream.hpp"
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QWKQuick/qwkquickglobal.h>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(AnoMap_FrontPlugin)


int main(int argc, char **argv) {
  QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);

  QGuiApplication app(argc, argv);
  QIcon appIcon(":/qt/qml/AnoMap/Front/Assets/Core/Logos/OnlyLogo.svg");
  if (!appIcon.isNull()) {
    app.setWindowIcon(appIcon);
  }
  QQmlApplicationEngine engine;
  QWK::registerTypes(&engine);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  // VideoStream is a QObject ??must be registered before QML load
  qmlRegisterType<VideoStream>("AnoMap.back", 1, 0, "VideoStream");

  Core core;
  core.init(engine);

  QObject::connect(&app, &QGuiApplication::aboutToQuit,
                   [&] { core.shutdown(); });

  // ── 4. QML import paths + load
  // ────────────────────────────────────────────────────────────────────────────────
  // Qt 6 standardizes plugin and module locations, so explicit addImportPath is
  // rarely needed.
  engine.loadFromModule("AnoMap.Front", "Main");

  return app.exec();
}
