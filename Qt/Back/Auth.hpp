#pragma once
#include "NetworkBridge.hpp"
#include <QObject>
#include <QString>

class AuthController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(bool isError READ isError NOTIFY isErrorChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
public:
  explicit AuthController(NetworkBridge *bridge, const QString &host,
                          const QString &port, QObject *parent = nullptr);

  bool isLoading() const { return m_isLoading; }
  bool isError() const { return m_isError; }
  QString errorMessage() const { return m_errorMessage; }

signals:
  void isLoadingChanged();
  void isErrorChanged();
  void errorMessageChanged();

protected:
  virtual void onConnected() = 0;
  void setLoading(bool v);
  void setError(const QString &msg);
  void clearError();

  NetworkBridge *m_bridge;
  QString m_host;
  QString m_port;
  bool m_isLoading = false;
  bool m_isError = false;
  QString m_errorMessage;
};

class LoginController : public AuthController {
  Q_OBJECT
  Q_PROPERTY(QString state READ state NOTIFY stateChanged)
  Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
public:
  explicit LoginController(NetworkBridge *bridge, const QString &host,
                           const QString &port, QObject *parent = nullptr);

  QString state() const { return m_state; }
  QString username() const { return m_username; }

  Q_INVOKABLE void login(const QString &id, const QString &password);
  Q_INVOKABLE void logout();

public slots:
  void handleLoginSuccess(QString state, QString username);
  void handleLoginFailed(QString error);

signals:
  void loginSuccess();
  void logoutRequested();
  void stateChanged();
  void usernameChanged();

protected:
  void onConnected() override;

private:
  QString m_state;
  QString m_username;
  QString m_pendingId;
  QString m_pendingPassword;
};

class SignupController : public AuthController {
  Q_OBJECT
public:
  explicit SignupController(NetworkBridge *bridge, const QString &host,
                            const QString &port, QObject *parent = nullptr);

  Q_INVOKABLE void signup(const QString &id, const QString &email,
                          const QString &password);

public slots:
  void handleSignupSuccess(QString message);
  void handleSignupFailed(QString error);

signals:
  void signupSuccess(QString message);

protected:
  void onConnected() override;

private:
  QString m_pendingId;
  QString m_pendingEmail;
  QString m_pendingPassword;
};
