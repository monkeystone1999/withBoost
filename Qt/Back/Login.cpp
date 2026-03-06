#include "Login.hpp"
#include <QDebug>

Login::Login(NetworkBridge *bridge, QObject *parent)
    : QObject(parent), m_bridge(bridge) {
  if (m_bridge) {
    connect(m_bridge, &NetworkBridge::loginSuccess, this,
            &Login::handleLoginSuccess);
    connect(m_bridge, &NetworkBridge::loginFailed, this,
            &Login::handleLoginFailed);
    connect(m_bridge, &NetworkBridge::connectedToServer, this,
            &Login::onConnected);
  }
}

bool Login::isLoading() const { return m_isLoading; }
bool Login::isError() const { return m_isError; }
QString Login::errorMessage() const { return m_errorMessage; }
QString Login::state() const { return m_state; }
QString Login::username() const { return m_username; }

void Login::login(const QString &id, const QString &password) {
  if (id.isEmpty() || password.isEmpty()) {
    m_isError = true;
    m_errorMessage = "ID and Password cannot be empty";
    emit isErrorChanged();
    emit errorMessageChanged();
    return;
  }

  m_isError = false;
  m_errorMessage = "";
  emit isErrorChanged();
  emit errorMessageChanged();

  m_isLoading = true;
  emit isLoadingChanged();

  m_pendingId = id;
  m_pendingPassword = password;

  if (m_bridge) {
    if (m_bridge->hasSession("Main")) {
      qDebug() << "[Login] [1/1] Session 'Main' already exists — skipping TCP "
                  "connect";
      onConnected();
    } else {
      qDebug() << "[Login] [1/3] No active session — initiating TCP connect";
      qDebug() << "[Login] [2/3] Target: 192.168.0.58:20000";
      m_bridge->connectToServer("Main", "192.168.0.58", "20000");
      qDebug() << "[Login] [3/3] connectToServer() called — waiting for "
                  "onConnected signal...";
    }
  } else {
    qDebug() << "[Login] ERROR: m_bridge is null — cannot connect";
  }
}

void Login::onConnected() {
  qDebug() << "[Login] onConnected() called, pendingId:" << m_pendingId;
  if (m_bridge && !m_pendingId.isEmpty()) {
    qDebug() << "[Login] Sending login packet...";
    m_bridge->sendLogin(m_pendingId, m_pendingPassword);
    m_pendingId.clear();
    m_pendingPassword.clear();
  }
}

void Login::handleLoginSuccess(QString state, QString username) {
  qDebug() << "[Login] SUCCESS — state:" << state << "username:" << username;
  m_isLoading = false;
  emit isLoadingChanged();

  m_state = state;
  emit stateChanged();

  m_username = username;
  emit usernameChanged();

  emit loginSuccess();
}

void Login::handleLoginFailed(QString error) {
  qDebug() << "[Login] FAILED —" << error;
  m_isLoading = false;
  emit isLoadingChanged();

  // 로그인 실패 시 소켓 연결 즉시 해제 — 다음 시도는 새 연결로
  if (m_bridge) {
    qDebug() << "[Login] Disconnecting session 'Main' after failure";
    m_bridge->disconnectFromServer("Main");
  }

  m_isError = true;
  m_errorMessage = error;
  emit isErrorChanged();
  emit errorMessageChanged();
}

void Login::logout() {
  qDebug() << "[Login] Logging out... disconnecting Main session.";
  if (m_bridge) {
    m_bridge->disconnectFromServer("Main");
  }

  // 로그인 관련 데이터 초기화
  m_state.clear();
  emit stateChanged();

  m_username.clear();
  emit usernameChanged();

  m_pendingId.clear();
  m_pendingPassword.clear();

  m_isLoading = false;
  emit isLoadingChanged();

  m_isError = false;
  m_errorMessage.clear();
  emit isErrorChanged();
  emit errorMessageChanged();
}
