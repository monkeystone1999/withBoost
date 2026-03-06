#pragma once
#include "NetworkBridge.hpp"
#include <QObject>
#include <QString>

class Signup : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(bool isError READ isError NOTIFY isErrorChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
  explicit Signup(NetworkBridge *bridge, QObject *parent = nullptr);

  bool isLoading() const;
  bool isError() const;
  QString errorMessage() const;

  Q_INVOKABLE void signup(const QString &id, const QString &email,
                          const QString &password);

public slots:
  void handleSignupSuccess(QString message);
  void handleSignupFailed(QString error);
  void onConnected();

signals:
  void signupSuccess(QString message);
  void isLoadingChanged();
  void isErrorChanged();
  void errorMessageChanged();

private:
  NetworkBridge *m_bridge;
  bool m_isLoading = false;
  bool m_isError = false;
  QString m_errorMessage;
  QString m_pendingId;
  QString m_pendingEmail;
  QString m_pendingPassword;
};
