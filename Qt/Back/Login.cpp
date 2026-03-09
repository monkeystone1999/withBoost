#include "Login.hpp"
#include <QDebug>

Login::Login(NetworkBridge *bridge,
             const QString &host,
             const QString &port,
             QObject *parent)
    : QObject(parent), m_bridge(bridge), m_host(host), m_port(port) {
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
    if (m_bridge->isConnected()) {
      qDebug() << "[Login] Session already connected — sending login directly";
      onConnected();
    } else {
      qDebug() << "[Login] Connecting to" << m_host << m_port;
      m_bridge->connectToServer(m_host, m_port);
    }
  } else {
    qDebug() << "[Login] ERROR: m_bridge is null";
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
    qDebug() << "[Login] Disconnecting after failure";
    m_bridge->disconnect();
  }

  m_isError = true;
  m_errorMessage = error;
  emit isErrorChanged();
  emit errorMessageChanged();
}

void Login::logout() {
  qDebug() << "[Login] Logging out — disconnecting";
  if (m_bridge) {
    m_bridge->disconnect();
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
