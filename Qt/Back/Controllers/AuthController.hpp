#pragma once
#include "Services/NetworkBridge.hpp"
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

  bool isLoading() const { return isLoading_; }
  bool isError() const { return isError_; }
  QString errorMessage() const { return errorMessage_; }

signals:
  void isLoadingChanged();
  void isErrorChanged();
  void errorMessageChanged();

protected:
  virtual void onConnected() = 0;
  void setLoading(bool v);
  void setError(const QString &msg);
  void clearError();

  NetworkBridge *bridge_;
  QString host_;
  QString port_;
  bool isLoading_ = false;
  bool isError_ = false;
  QString errorMessage_;
};

class LoginController : public AuthController {
  Q_OBJECT
  Q_PROPERTY(QString state READ state NOTIFY stateChanged)
  Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
public:
  explicit LoginController(NetworkBridge *bridge, const QString &host,
                           const QString &port, QObject *parent = nullptr);

  QString state() const { return state_; }
  QString username() const { return username_; }

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
  QString state_;
  QString username_;
  QString pendingId_;
  QString pendingPassword_;
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
  QString pendingId_;
  QString pendingEmail_;
  QString pendingPassword_;
};
