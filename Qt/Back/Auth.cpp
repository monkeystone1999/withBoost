#include "Auth.hpp"
#include <QDebug>

AuthController::AuthController(NetworkBridge *bridge, const QString &host,
                               const QString &port, QObject *parent)
    : QObject(parent), m_bridge(bridge), m_host(host), m_port(port) {}

void AuthController::setLoading(bool v) {
  if (m_isLoading == v)
    return;
  m_isLoading = v;
  emit isLoadingChanged();
}

void AuthController::setError(const QString &msg) {
  m_isError = true;
  m_errorMessage = msg;
  emit isErrorChanged();
  emit errorMessageChanged();
}

void AuthController::clearError() {
  m_isError = false;
  m_errorMessage.clear();
  emit isErrorChanged();
  emit errorMessageChanged();
}

// ------------------------------------------------------------------

LoginController::LoginController(NetworkBridge *bridge, const QString &host,
                                 const QString &port, QObject *parent)
    : AuthController(bridge, host, port, parent) {
  if (m_bridge) {
    connect(m_bridge, &NetworkBridge::loginSuccess, this,
            &LoginController::handleLoginSuccess);
    connect(m_bridge, &NetworkBridge::loginFailed, this,
            &LoginController::handleLoginFailed);
    connect(m_bridge, &NetworkBridge::connectedForLogin, this,
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

  m_pendingId = id;
  m_pendingPassword = password;

  if (m_bridge) {
    if (m_bridge->isConnected()) {
      qDebug() << "[Login] Session already connected — sending login directly";
      onConnected();
    } else {
      qDebug() << "[Login] Connecting to" << m_host << m_port;
      m_bridge->connectToServer(m_host, m_port, "login");
    }
  } else {
    qDebug() << "[Login] ERROR: m_bridge is null";
  }
}

void LoginController::onConnected() {
  qDebug() << "[Login] onConnected() called, pendingId:" << m_pendingId;
  if (m_bridge && !m_pendingId.isEmpty()) {
    qDebug() << "[Login] Sending login packet...";
    m_bridge->sendLogin(m_pendingId, m_pendingPassword);
    m_pendingId.clear();
    m_pendingPassword.clear();
  }
}

void LoginController::handleLoginSuccess(QString state, QString username) {
  qDebug() << "[Login] SUCCESS — state:" << state << "username:" << username;
  setLoading(false);

  m_state = state;
  emit stateChanged();

  m_username = username;
  emit usernameChanged();

  emit loginSuccess();
}

void LoginController::handleLoginFailed(QString error) {
  qDebug() << "[Login] FAILED —" << error;
  setLoading(false);

  if (m_bridge) {
    qDebug() << "[Login] Disconnecting after failure";
    m_bridge->disconnect();
  }

  setError(error);
}

void LoginController::logout() {
  qDebug() << "[Login] Logging out — disconnecting";
  if (m_bridge) {
    m_bridge->disconnect();
  }

  m_state.clear();
  emit stateChanged();

  m_username.clear();
  emit usernameChanged();

  m_pendingId.clear();
  m_pendingPassword.clear();

  setLoading(false);
  clearError();

  emit logoutRequested();
}

// ------------------------------------------------------------------

SignupController::SignupController(NetworkBridge *bridge, const QString &host,
                                   const QString &port, QObject *parent)
    : AuthController(bridge, host, port, parent) {
  if (m_bridge) {
    connect(m_bridge, &NetworkBridge::signupSuccess, this,
            &SignupController::handleSignupSuccess);
    connect(m_bridge, &NetworkBridge::signupFailed, this,
            &SignupController::handleSignupFailed);
    connect(m_bridge, &NetworkBridge::connectedForSignup, this,
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

  m_pendingId = id;
  m_pendingEmail = email;
  m_pendingPassword = password;

  if (m_bridge) {
    if (m_bridge->isConnected()) {
      qDebug() << "[Signup] Already connected — sending signup directly";
      onConnected();
    } else {
      qDebug() << "[Signup] Connecting to" << m_host << m_port;
      m_bridge->connectToServer(m_host, m_port, "signup");
    }
  }
}

void SignupController::onConnected() {
  qDebug() << "[Signup] onConnected() called, pendingId:" << m_pendingId;
  if (m_bridge && !m_pendingId.isEmpty()) {
    qDebug() << "[Signup] Sending signup packet...";
    m_bridge->sendSignup(m_pendingId, m_pendingEmail, m_pendingPassword);
    m_pendingId.clear();
    m_pendingEmail.clear();
    m_pendingPassword.clear();
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
