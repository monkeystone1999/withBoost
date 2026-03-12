#include "Auth.hpp"
#include <QDebug>

AuthController::AuthController(NetworkBridge *bridge, const QString &host,
                               const QString &port, QObject *parent)
    : QObject(parent), bridge_(bridge), host_(host), port_(port) {}

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

LoginController::LoginController(NetworkBridge *bridge, const QString &host,
                                 const QString &port, QObject *parent)
    : AuthController(bridge, host, port, parent) {
  if (bridge_) {
    connect(bridge_, &NetworkBridge::loginSuccess, this,
            &LoginController::handleLoginSuccess);
    connect(bridge_, &NetworkBridge::loginFailed, this,
            &LoginController::handleLoginFailed);
    connect(bridge_, &NetworkBridge::connectedForLogin, this,
            &LoginController::onConnected);
  }
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
    if (bridge_->isConnected()) {
      qDebug() << "[Login] Session already connected — sending login directly";
      onConnected();
    } else {
      qDebug() << "[Login] Connecting to" << host_ << port_;
      bridge_->connectToServer(host_, port_, "login");
    }
  } else {
    qDebug() << "[Login] ERROR: bridge_ is null";
  }
}

void LoginController::onConnected() {
  qDebug() << "[Login] onConnected() called, pendingId:" << pendingId_;
  if (bridge_ && !pendingId_.isEmpty()) {
    qDebug() << "[Login] Sending login packet...";
    bridge_->sendLogin(pendingId_, pendingPassword_);
    pendingId_.clear();
    pendingPassword_.clear();
  }
}

void LoginController::handleLoginSuccess(QString state, QString username) {
  qDebug() << "[Login] SUCCESS — state:" << state << "username:" << username;
  setLoading(false);

  state_ = state;
  emit stateChanged();

  username_ = username;
  emit usernameChanged();

  emit loginSuccess();
}

void LoginController::handleLoginFailed(QString error) {
  qDebug() << "[Login] FAILED —" << error;
  setLoading(false);

  if (bridge_) {
    qDebug() << "[Login] Disconnecting after failure";
    bridge_->disconnect();
  }

  setError(error);
}

void LoginController::logout() {
  qDebug() << "[Login] Logging out — disconnecting";
  if (bridge_) {
    bridge_->disconnect();
  }

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

SignupController::SignupController(NetworkBridge *bridge, const QString &host,
                                   const QString &port, QObject *parent)
    : AuthController(bridge, host, port, parent) {
  if (bridge_) {
    connect(bridge_, &NetworkBridge::signupSuccess, this,
            &SignupController::handleSignupSuccess);
    connect(bridge_, &NetworkBridge::signupFailed, this,
            &SignupController::handleSignupFailed);
    connect(bridge_, &NetworkBridge::connectedForSignup, this,
            &SignupController::onConnected);
  }
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

  if (bridge_) {
    if (bridge_->isConnected()) {
      qDebug() << "[Signup] Already connected — sending signup directly";
      onConnected();
    } else {
      qDebug() << "[Signup] Connecting to" << host_ << port_;
      bridge_->connectToServer(host_, port_, "signup");
    }
  }
}

void SignupController::onConnected() {
  qDebug() << "[Signup] onConnected() called, pendingId:" << pendingId_;
  if (bridge_ && !pendingId_.isEmpty()) {
    qDebug() << "[Signup] Sending signup packet...";
    bridge_->sendSignup(pendingId_, pendingEmail_, pendingPassword_);
    pendingId_.clear();
    pendingEmail_.clear();
    pendingPassword_.clear();
  }
}

void SignupController::handleSignupSuccess(QString message) {
  qDebug() << "[Signup] SUCCESS —" << message;
  setLoading(false);
  emit signupSuccess(message);
}

void SignupController::handleSignupFailed(QString error) {
  qDebug() << "[Signup] FAILED —" << error;
  setLoading(false);
  setError(error);
}
