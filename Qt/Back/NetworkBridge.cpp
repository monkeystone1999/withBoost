#include "NetworkBridge.hpp"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

NetworkBridge::NetworkBridge(QObject *parent) : QObject(parent) {
    m_manager = std::make_unique<SessionManager>();
}

NetworkBridge::~NetworkBridge() {}

void NetworkBridge::connectToServer(const QString &name, const QString &ip,
                                    const QString &port) {
    m_manager->AddSession(name.toStdString(), ip.toStdString(), port.toStdString());
    auto session = m_manager->GetSession(name.toStdString());
    if (!session) return;

    session->onConnected = [this]() {
        QMetaObject::invokeMethod(this, [this]() { emit connectedToServer(); }, Qt::QueuedConnection);
    };
    session->onCameraMessage = [this](const std::string &s) {
        QMetaObject::invokeMethod(this, [this, q = QString::fromStdString(s)]() {
            emit cameraListReceived(q);
        }, Qt::QueuedConnection);
    };
    session->onAiMessage = [this](const std::string &s) {
        QMetaObject::invokeMethod(this, [this, q = QString::fromStdString(s)]() {
            emit aiResultReceived(q);
        }, Qt::QueuedConnection);
    };
    session->onSuccess = [this](const std::string &s) {
        QMetaObject::invokeMethod(this, [this, q = QString::fromStdString(s)]() {
            qDebug() << "[NetworkBridge] onSuccess:" << q;
            QJsonDocument doc = QJsonDocument::fromJson(q.toUtf8());
            if (!doc.isObject()) return;
            QJsonObject obj = doc.object();
            if (obj.contains("state"))
                emit loginSuccess(obj["state"].toString(), obj["username"].toString());
            else
                emit signupSuccess(obj["message"].toString());
        }, Qt::QueuedConnection);
    };
    session->onFail = [this](const std::string &s) {
        QMetaObject::invokeMethod(this, [this, q = QString::fromStdString(s)]() {
            qDebug() << "[NetworkBridge] onFail:" << q;
            QJsonDocument doc = QJsonDocument::fromJson(q.toUtf8());
            if (!doc.isObject()) { emit loginFailed(q); return; }
            QJsonObject obj = doc.object();
            QString msg = obj.contains("error") ? obj["error"].toString() : obj["message"].toString();
            QString ctx = obj["context"].toString();
            if (ctx == "login" || msg.contains("비밀번호") || msg.contains("존재하지 않는"))
                emit loginFailed(msg);
            else
                emit signupFailed(msg);
        }, Qt::QueuedConnection);
    };
    session->onAvailable = [this](const std::string &s) {
        QMetaObject::invokeMethod(this, [this, q = QString::fromStdString(s)]() {
            emit availableReceived(q);
        }, Qt::QueuedConnection);
    };
    session->onAssignMessage = [this](const std::string &s) {
        QMetaObject::invokeMethod(this, [this, q = QString::fromStdString(s)]() {
            emit assignReceived(q);
        }, Qt::QueuedConnection);
    };
    // 서버에서 DEVICE 헤더로 디바이스 목록을 보내면 deviceListReceived 로 전달
    session->onDeviceMessage = [this](const std::string &s) {
        QMetaObject::invokeMethod(this, [this, q = QString::fromStdString(s)]() {
            qDebug() << "[NetworkBridge] deviceListReceived:" << q;
            emit deviceListReceived(q);
        }, Qt::QueuedConnection);
    };

    session->StartConnect();
}

bool NetworkBridge::hasSession(const QString &name) {
    return m_manager->GetSession(name.toStdString()) != nullptr;
}
void NetworkBridge::disconnectFromServer(const QString &name) {
    m_manager->RemoveSession(name.toStdString());
}
void NetworkBridge::disconnectAll() {
    m_manager->CloseAll();
    m_manager = std::make_unique<SessionManager>();
}

void NetworkBridge::sendMessage(const QString &name, const QString &type,
                                const QString &jsonBody) {
    auto session = m_manager->GetSession(name.toStdString());
    if (!session) return;
    MessageType msgType = MessageType::CAMERA;
    if      (type == "CAMERA") msgType = MessageType::CAMERA;
    else if (type == "AI")     msgType = MessageType::AI;
    else if (type == "LOGIN")  msgType = MessageType::LOGIN;
    else if (type == "ASSIGN") msgType = MessageType::ASSIGN;
    else if (type == "DEVICE") msgType = MessageType::DEVICE;
    session->WriteRaw(msgType, jsonBody.toStdString());
}

void NetworkBridge::sendLogin(const QString &id, const QString &pw) {
    QJsonObject o; o["id"] = id; o["pw"] = pw;
    sendMessage("Main", "LOGIN", QJsonDocument(o).toJson(QJsonDocument::Compact));
}
void NetworkBridge::sendSignup(const QString &id, const QString &email, const QString &pw) {
    QJsonObject o; o["action"]="register"; o["id"]=id; o["email"]=email; o["pw"]=pw;
    sendMessage("Main", "ASSIGN", QJsonDocument(o).toJson(QJsonDocument::Compact));
}
void NetworkBridge::sendListPending() {
    QJsonObject o; o["action"]="list_pending";
    sendMessage("Main", "ASSIGN", QJsonDocument(o).toJson(QJsonDocument::Compact));
}
void NetworkBridge::sendApprove(const QString &targetId) {
    QJsonObject o; o["action"]="approve"; o["target_username"]=targetId;
    sendMessage("Main", "ASSIGN", QJsonDocument(o).toJson(QJsonDocument::Compact));
}

// ── Device 제어 패킷 ──────────────────────────────────────────────────────────
// 사용자가 GUI 에서 건드린 항목만 JSON 에 포함
// ex) motor 만 누름:   { "device":"192.168.0.23", "motor":"w" }
// ex) ir 토글:         { "device":"192.168.0.23", "ir":"on" }
// ex) motor+heater:    { "device":"192.168.0.23", "motor":"auto", "heater":"off" }
void NetworkBridge::sendDevice(const QString &deviceIp,
                               const QString &motor,
                               const QString &ir,
                               const QString &heater) {
    if (deviceIp.isEmpty()) return;
    QJsonObject body;
    body["device"] = deviceIp;
    if (!motor.isEmpty())  body["motor"]  = motor;
    if (!ir.isEmpty())     body["ir"]     = ir;
    if (!heater.isEmpty()) body["heater"] = heater;
    const QString json = QJsonDocument(body).toJson(QJsonDocument::Compact);
    qDebug() << "[NetworkBridge] sendDevice ->" << json;
    sendMessage("Main", "DEVICE", json);
}
