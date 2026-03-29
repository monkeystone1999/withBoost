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
  } else {
    qWarning() << "[AppController] failed to create CameraWindow object.";
  }
}
