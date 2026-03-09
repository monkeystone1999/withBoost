#pragma once
#include "../Src/Network/INetworkService.hpp"
#include <QObject>
#include <QString>

// ============================================================
//  NetworkBridge — Qt type bridge over INetworkService
//
//  RULE:  This class does ONE thing: convert QString <-> std::string.
//         It must NOT:
//           - own SessionManager or any infrastructure
//           - parse JSON beyond reading 2-3 flat string fields
//           - decide server address or port (injected via connect())
//           - create or connect signals of other QObjects
//
//  WHY:   Previously NetworkBridge owned SessionManager, hard-coded
//         session name, and parsed JSON inside lambda callbacks.
//         After DI refactor it holds only a non-owning pointer to
//         INetworkService (owned by Core).
// ============================================================

class NetworkBridge : public QObject {
  Q_OBJECT
public:
  // Injected — Core owns the service, NetworkBridge only references it
  explicit NetworkBridge(INetworkService *service, QObject *parent = nullptr);

  // Called by Core::init() after construction to register callbacks
  void wireCallbacks();

  // ── QML-invokable commands ───────────────────────────────
  // Connect to the server — host and port come from Core (Config)
  Q_INVOKABLE void connectToServer(const QString &host, const QString &port);
  Q_INVOKABLE void disconnect();
  // isConnected: replaces old hasSession() — no session names exposed to Qt
  // layer
  Q_INVOKABLE bool isConnected() const;

  Q_INVOKABLE void sendLogin(const QString &id, const QString &pw);
  Q_INVOKABLE void sendSignup(const QString &id, const QString &email,
                              const QString &pw);
  Q_INVOKABLE void sendListPending();
  Q_INVOKABLE void sendApprove(const QString &targetId);
  Q_INVOKABLE void sendDevice(const QString &deviceIp, const QString &motor,
                              const QString &ir, const QString &heater);

signals:
  void connectedToServer();
  void loginSuccess(QString state, QString username);
  void loginFailed(QString errorMessage);
  void signupSuccess(QString message);
  void signupFailed(QString errorMessage);
  void cameraListReceived(QString json); // raw JSON forwarded to Core
  void deviceStatusReceived(QString json);
  void aiResultReceived(QString json);
  void assignReceived(QString json);

private:
  INetworkService *m_service; // non-owning
};
