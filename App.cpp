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
#include "Qt/Back/VideoStream.hpp"
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

  // VideoStream is a QObject — must be registered before QML load
  qmlRegisterType<VideoStream>("AnoMap.back", 1, 0, "VideoStream");

  // ── 3. Core: constructs everything, wires signals, registers QML props ─
  Core core;
  core.init(engine);

  QObject::connect(&app, &QGuiApplication::aboutToQuit,
                   [&] { core.shutdown(); });

  // ── 4. QML import paths + load ────────────────────────────────────────
  engine.addImportPath("qrc:/qt/qml");
  engine.addImportPath(":/qt/qml");
  engine.addImportPath("qrc:/");
  engine.loadFromModule("AnoMap.front", "Main");

  return app.exec();
}
