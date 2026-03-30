#include "AuthController.hpp"
#include "../../../Src/Domain/AuthBridge.hpp"
#include <nlohmann/json.hpp>

AuthController::AuthController(const QString &host, const QString &port,
                               class AuthBridge *bridge, QObject *parent)
    : QObject(parent), host_(host), port_(port), bridge_(bridge) {}

void AuthController::setLoading(bool v) {
  if (isLoading_ == v)
    return;
  isLoading_ = v;
  emit isLoadingChanged();
}

void AuthController::setError(const QString &msg) {
  isError_ = true;
  errorMessage_ = msg;
  emit isErrorChanged();
  emit errorMessageChanged();
}

void AuthController::clearError() {
  isError_ = false;
  errorMessage_.clear();
  emit isErrorChanged();
  emit errorMessageChanged();
}

// ------------------------------------------------------------------

LoginController::LoginController(const QString &host, const QString &port,
                                 class AuthBridge *bridge, QObject *parent)
    : AuthController(host, port, bridge, parent) {
  // [DEPRECATED] NetworkBridge 시그널 연결
  /*
  if (bridge_) {
    connect(bridge_, &NetworkBridge::loginSuccess, this,
  &LoginController::handleLoginSuccess); connect(bridge_,
  &NetworkBridge::loginFailed, this, &LoginController::handleLoginFailed);
    connect(bridge_, &NetworkBridge::connectedForLogin, this,
  &LoginController::onConnected);
  }
  */
}

void LoginController::login(const QString &id, const QString &password) {
  if (id.isEmpty() || password.isEmpty()) {
    setError("ID and Password cannot be empty");
    return;
  }
  clearError();
  setLoading(true);

  pendingId_ = id;
  pendingPassword_ = password;

  if (bridge_) {
    onConnected();
  }
}

void LoginController::onConnected() {
  if (bridge_ && !pendingId_.isEmpty()) {
    bridge_->login(pendingId_.toStdString(), pendingPassword_.toStdString());
    pendingId_.clear();
    pendingPassword_.clear();
  }
}

void LoginController::handleLoginSuccess(QString state, QString username) {
  setLoading(false);
  state_ = state;
  emit stateChanged();
  username_ = username;
  emit usernameChanged();
  emit loginSuccess();
}

void LoginController::handleLoginFailed(QString error) {
  setLoading(false);
  setError(error);
}

void LoginController::logout() {
  state_.clear();
  emit stateChanged();
  username_.clear();
  emit usernameChanged();
  pendingId_.clear();
  pendingPassword_.clear();
  setLoading(false);
  clearError();
  emit logoutRequested();
}

// ------------------------------------------------------------------

SignupController::SignupController(const QString &host, const QString &port,
                                   class AuthBridge *bridge, QObject *parent)
    : AuthController(host, port, bridge, parent) {
  // [DEPRECATED] NetworkBridge 시그널 연결
  /*
  if (bridge_) {
    connect(bridge_, &NetworkBridge::signupSuccess, this,
  &SignupController::handleSignupSuccess); connect(bridge_,
  &NetworkBridge::signupFailed, this, &SignupController::handleSignupFailed);
    connect(bridge_, &NetworkBridge::connectedForSignup, this,
  &SignupController::onConnected);
  }
  */
}

void SignupController::signup(const QString &id, const QString &email,
                              const QString &password) {
  if (id.isEmpty() || email.isEmpty() || password.isEmpty()) {
    setError("모든 항목을 입력해야 합니다.");
    return;
  }
  clearError();
  setLoading(true);
  pendingId_ = id;
  pendingEmail_ = email;
  pendingPassword_ = password;
  // TODO: ServerConnect::Send(MessageType::ASSIGN, ...) 호출
}

void SignupController::onConnected() {
  // TODO: ServerConnect 기반 전환
}

void SignupController::handleSignupSuccess(QString message) {
  setLoading(false);
  emit signupSuccess(message);
}

void SignupController::handleSignupFailed(QString error) {
  setLoading(false);
  setError(error);
}

/**
 * @section Workflow Guide
 *
 * **[AuthController 구현 세부 가이드]**
 *
 * 1. 비동기 무결성 유지:
 *    - `pendingId_`, `pendingPassword_` 등 `pending` 멤버들은 비동기 서버 연결
 * 시점(`onConnected`)까지 사용자 입력을 보관하여 컨텍스트 분실을 방지합니다.
 *
 * 2. 에러 핸들링 패턴:
 *    - 모든 실패 시나리오(`handleLoginFailed`, `handleSignupFailed`)에서는
 * `setError`를 호출하여 UI 에러 배너를 즉시 활성화합니다.
 *    - 새 요청 시 `clearError`를 통해 이전 에러 상태를 명시적으로 초기화하는
 * 것이 표준 절차입니다.
 *
 * 3. 세션 클린업:
 *    - `logout()` 수행 시 모든 사용자 식별 프로퍼티를 비워(Clear), QML
 * 레이어에서 바인딩된 민감 정보 노출을 차단해야 합니다.
 */
