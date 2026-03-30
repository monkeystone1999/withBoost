/**
 * @file App.cpp
 * @brief 애플리케이션 진입점 및 Qt 엔진 기본 구성
 *
 * 이 파일은 Qt 런타임 환경 설정, 그래픽 API 선택, QML 엔진 초기화를 담당합니다.
 * 비즈니스 로직은 포함하지 않으며, 모든 오케스트레이션은 Core::init()에
 * 위임합니다.
 *
 * **주요 역할:**
 * 1. 렌더링 API (Direct3D11) 및 윈도우 프레임워크(QWindowKit) 설정.
 * 2. QML 플러그인 로드 및 VideoStream 등 필수 타입 등록.
 * 3. Core 인스턴스를 통한 기술 레이어 기동.
 */

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
