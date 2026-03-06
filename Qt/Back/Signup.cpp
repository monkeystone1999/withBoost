#include "Signup.hpp"
#include <QDebug>

Signup::Signup(NetworkBridge *bridge, QObject *parent)
    : QObject(parent), m_bridge(bridge) {
  if (m_bridge) {
    connect(m_bridge, &NetworkBridge::signupSuccess, this,
            &Signup::handleSignupSuccess);
    connect(m_bridge, &NetworkBridge::signupFailed, this,
            &Signup::handleSignupFailed);
    connect(m_bridge, &NetworkBridge::connectedToServer, this,
            &Signup::onConnected);
  }
}

bool Signup::isLoading() const { return m_isLoading; }
bool Signup::isError() const { return m_isError; }
QString Signup::errorMessage() const { return m_errorMessage; }

void Signup::signup(const QString &id, const QString &email,
                    const QString &password) {
  if (id.isEmpty() || email.isEmpty() || password.isEmpty()) {
    m_isError = true;
    m_errorMessage = "모든 항목을 입력해야 합니다.";
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
  m_pendingEmail = email;
  m_pendingPassword = password;

  if (m_bridge) {
    if (m_bridge->hasSession("Main")) {
      qDebug() << "[Signup] Session already exists, sending signup directly";
      onConnected();
    } else {
      qDebug() << "[Signup] No session, connecting to server...";
      m_bridge->connectToServer("Main", "192.168.0.52", "20000");
    }
  }
}

void Signup::onConnected() {
  qDebug() << "[Signup] onConnected() called, pendingId:" << m_pendingId;
  if (m_bridge && !m_pendingId.isEmpty()) {
    qDebug() << "[Signup] Sending signup packet...";
    m_bridge->sendSignup(m_pendingId, m_pendingEmail, m_pendingPassword);
    m_pendingId.clear();
    m_pendingEmail.clear();
    m_pendingPassword.clear();
  }
}

void Signup::handleSignupSuccess(QString message) {
  qDebug() << "[Signup] SUCCESS —" << message;
  m_isLoading = false;
  emit isLoadingChanged();

  emit signupSuccess(message);
}

void Signup::handleSignupFailed(QString error) {
  qDebug() << "[Signup] FAILED —" << error;
  m_isLoading = false;
  emit isLoadingChanged();

  m_isError = true;
  m_errorMessage = error;
  emit isErrorChanged();
  emit errorMessageChanged();
}
