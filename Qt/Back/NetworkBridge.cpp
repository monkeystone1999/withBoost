#include "NetworkBridge.hpp"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

// Helper: build JSON string from key/value pairs (avoids Qt JSON in hot path)
static std::string buildJson(std::initializer_list<std::pair<const char*, std::string>> pairs) {
    std::string s = "{";
    bool first = true;
    for (auto &[k, v] : pairs) {
        if (v.empty()) continue;
        if (!first) s += ',';
        s += '"'; s += k; s += "\":\""; s += v; s += '"';
        first = false;
    }
    s += '}';
    return s;
}

NetworkBridge::NetworkBridge(INetworkService *service, QObject *parent)
    : QObject(parent), m_service(service) {}

// Called by Core::init() after NetworkBridge is created.
// Registers all INetworkService callbacks — the only place where
// lambdas capture `this` for Qt signal emission.
void NetworkBridge::wireCallbacks() {
    // Core calls connectToServer() later; wireCallbacks only stores
    // the lambda factories.  Actual session callbacks are set inside
    // connectToServer() → NetworkService::connect().
    // (wireCallbacks is kept as an explicit init step for clarity.)
}

void NetworkBridge::connectToServer(const QString &host, const QString &port) {
    NetworkCallbacks cbs;

    cbs.onConnected = [this]() {
        QMetaObject::invokeMethod(this, [this] { emit connectedToServer(); },
                                  Qt::QueuedConnection);
    };

    // SUCCESS body: {"state":"admin","success":true,"username":"admin_alpha"}
    // Type-bridge only: read 3 flat string fields, emit signal.
    cbs.onLoginResult = [this](const std::string &s) {
        QMetaObject::invokeMethod(this, [this, q = QString::fromStdString(s)] {
            const auto doc = QJsonDocument::fromJson(q.toUtf8());
            if (!doc.isObject()) return;
            const auto obj = doc.object();
            if (obj.value("success").toBool(false))
                emit loginSuccess(obj.value("state").toString(),
                                  obj.value("username").toString());
            else
                emit loginFailed(obj.value("message").toString());
        }, Qt::QueuedConnection);
    };

    cbs.onLoginFail = [this](const std::string &s) {
        QMetaObject::invokeMethod(this, [this, q = QString::fromStdString(s)] {
            const auto doc = QJsonDocument::fromJson(q.toUtf8());
            const auto obj = doc.isObject() ? doc.object() : QJsonObject{};
            const QString msg = obj.contains("error")
                ? obj.value("error").toString()
                : obj.value("message").toString();
            // Context-based routing: login vs signup failure
            const QString ctx = obj.value("context").toString();
            if (ctx == "login" || msg.contains("비밀번호") ||
                msg.contains("존재하지 않는"))
                emit loginFailed(msg);
            else
                emit signupFailed(msg);
        }, Qt::QueuedConnection);
    };

    // For CAMERA / DEVICE / AI: just convert std::string → QString and emit.
    // All parsing happens in Domain stores on the ThreadPool (Core wires this).
    cbs.onCameraList = [this](const std::string &s) {
        QMetaObject::invokeMethod(this,
            [this, q = QString::fromStdString(s)] { emit cameraListReceived(q); },
            Qt::QueuedConnection);
    };

    cbs.onDeviceStatus = [this](const std::string &s) {
        QMetaObject::invokeMethod(this,
            [this, q = QString::fromStdString(s)] { emit deviceStatusReceived(q); },
            Qt::QueuedConnection);
    };

    cbs.onAiResult = [this](const std::string &s) {
        QMetaObject::invokeMethod(this,
            [this, q = QString::fromStdString(s)] { emit aiResultReceived(q); },
            Qt::QueuedConnection);
    };

    cbs.onAssign = [this](const std::string &s) {
        QMetaObject::invokeMethod(this,
            [this, q = QString::fromStdString(s)] { emit assignReceived(q); },
            Qt::QueuedConnection);
    };

    m_service->connect(host.toStdString(), port.toStdString(), cbs);
}

void NetworkBridge::disconnect() {
    m_service->disconnect();
}

void NetworkBridge::sendLogin(const QString &id, const QString &pw) {
    // Type conversion only: QString → std::string → raw JSON
    const auto body = buildJson({{"id", id.toStdString()}, {"pw", pw.toStdString()}});
    m_service->send(static_cast<uint8_t>(0x01), body); // MessageType::LOGIN
}

void NetworkBridge::sendSignup(const QString &id, const QString &email,
                               const QString &pw) {
    const auto body = buildJson({
        {"action", "register"},
        {"id",     id.toStdString()},
        {"email",  email.toStdString()},
        {"pw",     pw.toStdString()}
    });
    m_service->send(static_cast<uint8_t>(0x08), body); // ASSIGN
}

void NetworkBridge::sendListPending() {
    m_service->send(static_cast<uint8_t>(0x08), R"({"action":"list_pending"})");
}

void NetworkBridge::sendApprove(const QString &targetId) {
    const auto body = buildJson({
        {"action",          "approve"},
        {"target_username", targetId.toStdString()}
    });
    m_service->send(static_cast<uint8_t>(0x08), body);
}

void NetworkBridge::sendDevice(const QString &deviceIp,
                               const QString &motor,
                               const QString &ir,
                               const QString &heater) {
    if (deviceIp.isEmpty()) return;
    const auto body = buildJson({
        {"device", deviceIp.toStdString()},
        {"motor",  motor.toStdString()},
        {"ir",     ir.toStdString()},
        {"heater", heater.toStdString()}
    });
    qDebug() << "[NetworkBridge] sendDevice ->" << QString::fromStdString(body);
    m_service->send(static_cast<uint8_t>(0x04), body); // DEVICE
}

bool NetworkBridge::isConnected() const {
    return m_service->isConnected();
}
