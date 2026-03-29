#include "AuthController.hpp"

// [DEPRECATED] NetworkBridge 기반 생성자 — TODO: ServerConnect 기반으로 전환
AuthController::AuthController(/*NetworkBridge *bridge,*/ const QString &host,
                               const QString &port, QObject *parent)
    : QObject(parent), /*bridge_(bridge),*/ host_(host), port_(port) {}

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

LoginController::LoginController(/*NetworkBridge *bridge,*/ const QString &host,
                                 const QString &port, QObject *parent)
    : AuthController(/*bridge,*/ host, port, parent) {
  // [DEPRECATED] NetworkBridge 시그널 연결
  /*
  if (bridge_) {
    connect(bridge_, &NetworkBridge::loginSuccess, this, &LoginController::handleLoginSuccess);
    connect(bridge_, &NetworkBridge::loginFailed, this, &LoginController::handleLoginFailed);
    connect(bridge_, &NetworkBridge::connectedForLogin, this, &LoginController::onConnected);
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

  // TODO: ServerConnect::Send() 기반으로 전환
  /*
  if (bridge_) {
    if (bridge_->isConnected()) {
      onConnected();
    } else {
      bridge_->connectToServer(host_, port_, "login");
    }
  }
  */
}

void LoginController::onConnected() {
  // TODO: ServerConnect::Send(MessageType::LOGIN, ...) 호출
  /*
  if (bridge_ && !pendingId_.isEmpty()) {
    bridge_->sendLogin(pendingId_, pendingPassword_);
    pendingId_.clear();
    pendingPassword_.clear();
  }
  */
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

SignupController::SignupController(/*NetworkBridge *bridge,*/ const QString &host,
                                   const QString &port, QObject *parent)
    : AuthController(/*bridge,*/ host, port, parent) {
  // [DEPRECATED] NetworkBridge 시그널 연결
  /*
  if (bridge_) {
    connect(bridge_, &NetworkBridge::signupSuccess, this, &SignupController::handleSignupSuccess);
    connect(bridge_, &NetworkBridge::signupFailed, this, &SignupController::handleSignupFailed);
    connect(bridge_, &NetworkBridge::connectedForSignup, this, &SignupController::onConnected);
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
