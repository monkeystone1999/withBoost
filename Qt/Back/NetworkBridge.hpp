#pragma once
#include "Network/Session.hpp"
#include <QObject>
#include <QString>
#include <memory>

class NetworkBridge : public QObject {
    Q_OBJECT
public:
    explicit NetworkBridge(QObject *parent = nullptr);
    ~NetworkBridge() override;

    Q_INVOKABLE void connectToServer(const QString &name, const QString &ip, const QString &port);
    Q_INVOKABLE bool hasSession(const QString &name);
    Q_INVOKABLE void disconnectFromServer(const QString &name);
    Q_INVOKABLE void disconnectAll();
    Q_INVOKABLE void sendMessage(const QString &name, const QString &type, const QString &jsonBody);

    Q_INVOKABLE void sendLogin(const QString &id, const QString &password);
    Q_INVOKABLE void sendSignup(const QString &id, const QString &email, const QString &password);
    Q_INVOKABLE void sendListPending();
    Q_INVOKABLE void sendApprove(const QString &targetId);

    // ── Device 제어 ───────────────────────────────────────────────────────────
    // Header : DEVICE (0x04)
    // Body   : { "device":"<ip>", "motor":"w"|"a"|"s"|"d"|"auto"|"unauto",
    //            "ir":"on"|"off"|"auto"|"unauto",
    //            "heater":"on"|"off"|"auto"|"unauto" }
    // 사용자가 건드린 항목만 채워서 호출 — 빈 문자열 필드는 JSON 에 포함하지 않음
    Q_INVOKABLE void sendDevice(const QString &deviceIp,
                                const QString &motor,
                                const QString &ir,
                                const QString &heater);

signals:
    void cameraListReceived(const QString &jsonString);
    void deviceListReceived(const QString &jsonString); // 서버 → 디바이스 목록 (DEVICE 수신)
    void aiResultReceived(const QString &jsonString);
    void loginSuccess(QString state, QString username);
    void loginFailed(QString errorMessage);
    void signupSuccess(QString message);
    void signupFailed(QString errorMessage);
    void listPendingReceived(const QString &jsonString);
    void availableReceived(const QString &jsonString);
    void assignReceived(const QString &jsonString);
    void connectedToServer();

private:
    std::unique_ptr<SessionManager> m_manager;
};
