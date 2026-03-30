#pragma once
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

class QQmlEngine;

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
/**
 * @file AppController.hpp
 * @brief 애플리케이션 전역 상태 및 상위 레벨 네비게이션 컨트롤러
 *
 * 이 파일은 메인 화면의 페이지 전환(Navigation), 독립된 카메라 윈도우 관리,
 * 그리고 로그아웃 및 종료 프로세스를 총괄하는 라우터 기능을 정의합니다.
 */

class QQmlEngine;

/**
 * @class AppController
 * @brief 앱 네비게이션 및 다중 윈도우 수명 주기 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. navigate()를 호출하여 QML StackView의 현재 표시 페이지를 전환합니다.
 * 2. detachCameraWindow()를 사용하여 특정 카메라 채널을 별도의 외부 윈도우로
 * 분리합니다.
 * 3. logout() 또는 shutdown()을 통해 앱의 상태를 정리하고 세션을 종료합니다.
 */
class AppController : public QObject {
  Q_OBJECT
  /** @brief 현재 QML 메인 스택에 표시 중인 페이지 이름 */
  Q_PROPERTY(QString currentPage READ currentPage NOTIFY currentPageChanged)

public:
  explicit AppController(QObject *parent = nullptr);

  /** @return QString 현재 페이지 식별자 */
  QString currentPage() const { return currentPage_; }

  /**
   * @brief 페이지 네비게이션 요청
   * @param page 이동할 페이지 이름 (예: "Dashboard", "Settings")
   */
  Q_INVOKABLE void navigate(const QString &page);

  /** @brief 전역 로그아웃 처리 및 모든 하위 윈도우 폐쇄 */
  Q_INVOKABLE void logout();

  /** @brief 설정 다이얼로그 노출 요청 시그널 발생 */
  Q_INVOKABLE void openOptionDialog();

  /** @brief 런타임 윈도우 생성을 위한 QML 엔진 주입 */
  void setEngine(QQmlEngine *engine) { engine_ = engine; }

  /**
   * @brief 동적 생성된 카메라 윈도우 등록
   * @param win 관리 대상 QML Window 객체
   */
  Q_INVOKABLE void registerCameraWindow(QObject *win);

  /** @brief 카메라 윈도우 관리 대상에서 제외 */
  Q_INVOKABLE void unregisterCameraWindow(QObject *win);

  /** @brief 현재 열려 있는 모든 별도 카메라 윈도우 강제 종료 */
  Q_INVOKABLE void closeAllCameraWindows();

  /** @brief 시스템 종료 전 자원 정리 프로세스 */
  Q_INVOKABLE void shutdown();

  /**
   * @brief 카메라 채널을 별도 윈도우로 분리(Detach) 생성
   * @param slotId 그리드 내 원래 슬롯 번호
   * @param title 윈도우 제목
   * @param cameraId 카메라 고유 식별자
   * @param isOnline 네트워크 연결 상태
   * @param cropRect 확대/크롭 영역 정보
   * @param globalX 윈도우 초기 X 좌표
   * @param globalY 윈도우 초기 Y 좌표
   */
  Q_INVOKABLE void detachCameraWindow(int slotId, const QString &title,
                                      const QString &cameraId, bool isOnline,
                                      const QRectF &cropRect, int globalX,
                                      int globalY);

signals:
  /** @brief QML StackView에게 페이지 교체를 지시하는 시그널 */
  void navigateTo(const QString &page);

  /** @brief 현재 페이지 정보 갱신 시 통지 */
  void currentPageChanged();

  /** @brief QML 레이어에 옵션 팝업 노출 지시 */
  void optionDialogRequested();

  /** @brief 로그아웃 처리를 위한 전역 시그널 */
  void logoutRequested();

private:
  QString currentPage_{"Login"}; /**< 활성 페이지 백킹 스토어 */
  QList<QPointer<QObject>> cameraWindows_; /**< 동적 생성된 윈도우들에 대한 약한
                                              참조(Weak Reference) 리스트 */
  QQmlEngine *engine_ = nullptr; /**< 객체 생성을 위한 엔진 참조 */
};

/**
 * @section Workflow Guide
 *
 * **[AppController 네비게이션 워크플로우]**
 *
 * 1. 호출: 유저가 대시보드 버튼을 클릭하면 QML에서
 * `appController.navigate("Dashboard")`를 호출합니다.
 * 2. 전파: 내부 상태(`currentPage_`) 갱신 후 `navigateTo` 시그널이 발생합니다.
 * 3. 반응: `Main.qml`의 `StackView`가 해당 시그널을 감지하여 컴포넌트를
 * 푸시(Push) 또는 리플레이스(Replace)합니다.
 *
 * **[윈도우 독립 관리 워크플로우]**
 *
 * 1. 분리: 유저가 카메라 화면을 드래그하여 외부로 빼내면 `detachCameraWindow`가
 * 호출됩니다.
 * 2. 생성: 주입된 `engine_`을 사용하여 `CameraWindow.qml` 인스턴스를 동적으로
 * 생성하고 속성을 주입합니다.
 * 3. 관리: 생성된 윈도우 포인터를 `cameraWindows_`에 등록하여 일괄 종료(Close
 * All)가 가능하도록 트래킹합니다.
 */
