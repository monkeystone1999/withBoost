#include "AppController.hpp"
#include <QDebug>

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
  for (const auto &p : qAsConst(cameraWindows_)) {
    if (p == win)
      return;
  }
  cameraWindows_.append(QPointer<QObject>(win));
  qDebug() << "[AppController] registerCameraWindow, total:"
           << cameraWindows_.size();
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
      // QWindow::close() → QMetaObject로 호출 (QML Window도 동작)
      QMetaObject::invokeMethod(p, "close");
    }
  }
  cameraWindows_.clear();
}
