#pragma once
#include "NetworkBridge.hpp"
#include <QObject>
#include <QString>

class Login : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(bool isError READ isError NOTIFY isErrorChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
  Q_PROPERTY(QString state READ state NOTIFY stateChanged)
  Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
public:
  explicit Login(NetworkBridge *bridge, QObject *parent = nullptr);

  bool isLoading() const;
  bool isError() const;
  QString errorMessage() const;
  QString state() const;
  QString username() const;

  Q_INVOKABLE void login(const QString &id, const QString &password);
  Q_INVOKABLE void logout();

public slots:
  void handleLoginSuccess(QString state, QString username);
  void handleLoginFailed(QString error);
  void onConnected();

signals:
  void loginSuccess();
  void isLoadingChanged();
  void isErrorChanged();
  void errorMessageChanged();
  void stateChanged();
  void usernameChanged();

private:
  NetworkBridge *m_bridge;
  bool m_isLoading = false;
  bool m_isError = false;
  QString m_errorMessage;
  QString m_state;
  QString m_username;
  QString m_pendingId;
  QString m_pendingPassword;
};