/**
 * @file AuthController.hpp
 * @brief 인증(로그인, 회원가입) 프로세스 제어 및 상태 관리 컨트롤러
 *
 * 이 파일은 사용자 인증 과정에서 발생하는 네트워크 처리 상태(로딩, 에러
 * 메시지)와 실제 로그인/회원가입 요청 및 결과 핸들링을 위한 컨트롤러 클래스들을
 * 정의합니다.
 */

/**
 * @class AuthController
 * @brief 인증 관련 컨트롤러의 공통 기능을 추상화한 베이스 클래스
 */
#include <QObject>

class AuthController : public QObject {
  Q_OBJECT
  /** @brief 서버 응답 대기 중인 상태 여부 */
  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  /** @brief 인증 과정에서 오류 발생 여부 */
  Q_PROPERTY(bool isError READ isError NOTIFY isErrorChanged)
  /** @brief 사용자에게 표시할 에러 메시지 내용 */
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
  /**
   * @brief 생성자
   * @param host 접속 서버 호스트 주소
   * @param port 접속 서버 포트 번호
   */
  explicit AuthController(const QString &host, const QString &port,
                          class AuthBridge *bridge = nullptr,
                          QObject *parent = nullptr);

  /** @return bool 현재 비동기 작업 진행 중 여부 */
  bool isLoading() const { return isLoading_; }
  /** @return bool 에러 상태 여부 */
  bool isError() const { return isError_; }
  /** @return QString 에러 메시지 문자열 */
  QString errorMessage() const { return errorMessage_; }

signals:
  void isLoadingChanged();
  void isErrorChanged();
  void errorMessageChanged();

protected:
  /** @brief 서버 연결 완료 시 호출되는 추상 메서드 */
  virtual void onConnected() = 0;

  /** @brief 로딩 상태 변경 핸들러 */
  void setLoading(bool v);

  /** @brief 에러 발생 및 메시지 설정 */
  void setError(const QString &msg);

  /** @brief 에러 상태 초기화 */
  void clearError();

  QString host_; /**< 가동 중인 서버 IP/도메인 */
  QString port_; /**< 가동 중인 서버 포트 */
  class AuthBridge *bridge_ = nullptr;
  bool isLoading_ = false;
  bool isError_ = false;
  QString errorMessage_;
};

/**
 * @class LoginController
 * @brief 로그인 기능 특화 컨트롤러
 *
 * **Standard Usage Methodology:**
 * 1. login(id, password)를 호출하여 서버에 인증을 요청합니다.
 * 2. 성공 시 loginSuccess 시그널이 발생하며, QML 레이어는 메인 대시보드로
 * 이동합니다.
 * 3. 사용자 로그아웃 시 logout()을 호출하여 내부 세션 정량(Username, State)을
 * 정리합니다.
 */
class LoginController : public AuthController {
  Q_OBJECT
  /** @brief 로그인 완료 후 부여받은 사용자 상태 (직급 등) */
  Q_PROPERTY(QString state READ state NOTIFY stateChanged)
  /** @brief 현재 로그인한 사용자 이름 */
  Q_PROPERTY(QString username READ username NOTIFY usernameChanged)

public:
  explicit LoginController(const QString &host, const QString &port,
                           class AuthBridge *bridge = nullptr,
                           QObject *parent = nullptr);

  /** @return QString 사용자 권한/상태 */
  QString state() const { return state_; }
  /** @return QString 사용자 식별 이름 */
  QString username() const { return username_; }

  /**
   * @brief 로그인 실행
   * @param id 사용자 아이디
   * @param password 암호화 전 비밀번호
   */
  Q_INVOKABLE void login(const QString &id, const QString &password);

  /** @brief 현재 세션 로그아웃 처리 */
  Q_INVOKABLE void logout();

public slots:
  /** @brief 서버로부터 성공 응답 수신 시 데이터 동기화 */
  void handleLoginSuccess(QString state, QString username);
  /** @brief 서버로부터 실패 응답 수신 시 처리 */
  void handleLoginFailed(QString error);

signals:
  /** @brief 로그인 프로세스 최종 성공 통지 */
  void loginSuccess();
  /** @brief 로그아웃 수행 시 외부 레이어 통지 */
  void logoutRequested();
  /** @brief state 프로퍼티 변경 통지 */
  void stateChanged();
  /** @brief username 프로퍼티 변경 통지 */
  void usernameChanged();

protected:
  void onConnected() override;

private:
  QString state_;
  QString username_;
  QString pendingId_;
  QString pendingPassword_;
};

/**
 * @class SignupController
 * @brief 회원가입 기능 특화 컨트롤러
 *
 * **Standard Usage Methodology:**
 * 1. signup(id, email, password)를 호출하여 계정 생성을 요청합니다.
 * 2. 성공 시 signupSuccess 시그널을 통해 성공 메시지를 수신합니다.
 * 3. 반환된 메시지를 UI에 노출시킨 후 로그인 화면으로 전환(Stack.pop)합니다.
 */
class SignupController : public AuthController {
  Q_OBJECT
public:
  explicit SignupController(const QString &host, const QString &port,
                            class AuthBridge *bridge = nullptr,
                            QObject *parent = nullptr);

  /**
   * @brief 회원가입 실행
   * @param id 신규 아이디
   * @param email 연락처 이메일
   * @param password 설정 비밀번호
   */
  Q_INVOKABLE void signup(const QString &id, const QString &email,
                          const QString &password);

public slots:
  /** @brief 가입 성공 결과 수신 */
  void handleSignupSuccess(QString message);
  /** @brief 가입 실패 사유 수신 */
  void handleSignupFailed(QString error);

signals:
  /** @brief 최종 가입 완료 통지 */
  void signupSuccess(QString message);

protected:
  void onConnected() override;

private:
  QString pendingId_;
  QString pendingEmail_;
  QString pendingPassword_;
};

/**
 * @section Workflow Guide
 *
 * **[인증 시스템 통합 워크플로우]**
 *
 * 1. 요청 단계: QML 입력 폼 확인 -> `login()` 호출 -> `setLoading(true)`로 UI
 * 스피너 활성화.
 * 2. 통신 단계: `onConnected`에서 실제 패킷 전송 -> 비동기 응답 대기.
 * 3. 처리 단계: `handleLoginSuccess` 호출 -> `setLoading(false)` 및 세션 데이터
 * 프로퍼티 주입 -> `loginSuccess` 시그널 발생.
 * 4. UI 반응: QML에서 시그널 수신 후 `StackView` 페이지 전환.
 */
