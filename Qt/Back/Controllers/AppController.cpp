#include "AppController.hpp"
#include <QDebug>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QVariantMap>

AppController::AppController(QObject *parent) : QObject(parent) {}

void AppController::navigate(const QString &page) {
  if (currentPage_ == page)
    return;
  currentPage_ = page;
  emit currentPageChanged();
  emit navigateTo(page);
}

void AppController::logout() {
  closeAllCameraWindows();
  emit logoutRequested();
  navigate("Login");
}

void AppController::openOptionDialog() { emit optionDialogRequested(); }

void AppController::registerCameraWindow(QObject *win) {
  if (!win)
    return;
  // 이미 있으면 skip
  for (const auto &p : std::as_const(cameraWindows_)) {
    if (p == win)
      return;
  }
  cameraWindows_.append(QPointer<QObject>(win));
}

void AppController::unregisterCameraWindow(QObject *win) {
  for (int i = cameraWindows_.size() - 1; i >= 0; --i) {
    if (cameraWindows_[i] == win || cameraWindows_[i].isNull()) {
      cameraWindows_.removeAt(i);
    }
  }
}

void AppController::closeAllCameraWindows() {
  for (auto &p : cameraWindows_) {
    if (p && !p.isNull()) {
      // QWindow::close() → QMetaObject call (works for QML Window too)
      QMetaObject::invokeMethod(p, "close");
    }
  }
  cameraWindows_.clear();
}

void AppController::shutdown() { closeAllCameraWindows(); }

void AppController::detachCameraWindow(int slotId, const QString &title,
                                       const QString &cameraId, bool isOnline,
                                       const QRectF &cropRect, int globalX,
                                       int globalY) {
  QQmlEngine *engine = engine_;
  if (!engine) {
    qWarning() << "[AppController] no QQmlEngine found for detachCameraWindow!";
    return;
  }

  // Assuming module URI "AnoMap.Front" mapped to
  // "qrc:/qt/qml/AnoMap/Front"
  const QUrl url(QStringLiteral(
      "qrc:/qt/qml/AnoMap/Front/components/camera/CameraWindow.qml"));
  QQmlComponent comp(engine, url);
  if (comp.status() != QQmlComponent::Ready) {
    qWarning() << "[AppController] CameraWindow.qml not ready:"
               << comp.errors();
    return;
  }

  QVariantMap props;
  props["sourceSlotId"] = slotId;
  props["sourceTitle"] = title;
  props["sourceCameraId"] = cameraId;
  props["sourceOnline"] = isOnline;
  props["sourceCropRect"] = QVariant::fromValue(cropRect);

  QObject *winObj =
      comp.createWithInitialProperties(props, engine->rootContext());
  if (winObj) {
    auto *win = qobject_cast<QQuickWindow *>(winObj);
    if (win) {
      win->setX(globalX);
      win->setY(globalY);
      win->show();
    }
    qWarning() << "[AppController] failed to create CameraWindow object.";
  }
}

/**
 * @section Workflow Guide
 *
 * **[AppController 구현 상세 및 팁]**
 *
 * 1. QPointer의 활용:
 *    - `cameraWindows_`는 `QPointer<QObject>`를 담고 있어, QML 윈도우가
 * 사용자에 의해 수동으로 닫히거나 파괴되어도 댕글링 포인터 발생을 완벽하게
 * 차단합니다.
 *
 * 2. 동적 속성 주입:
 *    - `detachCameraWindow` 내부에서는
 * `QQmlComponent::createWithInitialProperties`를 사용하여 윈도우가 생성되는
 * 즉시 필요한 데이터(ID, RTSP 상태 등)를 안전하게 주입합니다.
 *
 * 3. 엔진 생명주기:
 *    - `engine_` 포인터는 `main.cpp`에서 주입되어야 합니다. 엔진이 유효하지
 * 않을 경우 경고를 출력하고 생성을 중단하여 앱 크래시를 방지합니다.
 */
