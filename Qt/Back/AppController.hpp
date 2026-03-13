#pragma once
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

// ============================================================
//  AppController — 앱 전역 상태 및 네비게이션 라우터
//
//  이전에 Main.qml JS에 흩어져 있던 로직:
//    - switch(pageName) 네비게이션 (2곳 중복)
//    - openCameraWindows 배열 관리
//    - logout (loginController + closeAllWindows)
//    - openOptionDialog
//  를 C++로 통합.
// ============================================================
class AppController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentPage READ currentPage NOTIFY currentPageChanged)

public:
  explicit AppController(QObject *parent = nullptr);

  QString currentPage() const { return currentPage_; }

  // ── QML Invokable ─────────────────────────────────────────
  // 페이지 전환 요청 — Main.qml의 StackView가 navigateTo를 받아서 처리
  Q_INVOKABLE void navigate(const QString &page);

  // 로그아웃: loginController.logout() 필요 시 별도 처리
  Q_INVOKABLE void logout();

  // 옵션 다이얼로그 열기
  Q_INVOKABLE void openOptionDialog();

  // CameraWindow 레지스트리 (QWindow* 대신 QObject*로 받아서 QML 호환)
  Q_INVOKABLE void registerCameraWindow(QObject *win);
  Q_INVOKABLE void unregisterCameraWindow(QObject *win);
  Q_INVOKABLE void closeAllCameraWindows();

signals:
  // QML StackView에서 이 시그널을 받아 replace/pop
  void navigateTo(const QString &page);
  void currentPageChanged();
  void optionDialogRequested();
  // logout 시 QML이 loginController.logout() 호출하도록
  void logoutRequested();

private:
  QString currentPage_{"Login"};
  QList<QPointer<QObject>> cameraWindows_;
};
