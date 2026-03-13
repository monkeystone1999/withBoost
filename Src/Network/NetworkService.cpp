#include "NetworkService.hpp"
#include "Header.hpp"
#include <cstdio>

NetworkService::NetworkService()
    : manager_(std::make_unique<SessionManager>()),
      sessionName_("Main") {}

NetworkService::~NetworkService() {
    disconnect();
}

void NetworkService::connect(const std::string &host,
                             const std::string &port,
                             NetworkCallbacks   cbs) {
    // Create (or recreate) the session
    manager_->AddSession(sessionName_, host, port);
    auto session = manager_->GetSession(sessionName_);
    if (!session) {
        fprintf(stderr, "[NetworkService] Failed to create session\n");
        return;
    }

    // Wire all callbacks — pure std::string, no Qt
    session->onConnected = [this, cb = cbs.onConnected]() {
        connected_ = true;
        if (cb) cb();
    };

    session->onSuccess = [cb = cbs.onLoginResult](const std::string &s) {
        if (cb) cb(s);
    };

    session->onFail = [cb = cbs.onLoginFail](const std::string &s) {
        if (cb) cb(s);
    };

    session->onCameraMessage = [cb = cbs.onCameraList](const std::string &s) {
        if (cb) cb(s);
    };

    session->onDeviceMessage = [cb = cbs.onDeviceStatus](const std::string &s) {
        if (cb) cb(s);
    };

    session->onAiMessage = [cb = cbs.onAiResult](const std::string &s) {
        if (cb) cb(s);
    };

    session->onAssignMessage = [cb = cbs.onAssign](const std::string &s) {
        if (cb) cb(s);
    };

    session->StartConnect();
}

void NetworkService::disconnect() {
    connected_ = false;
    manager_->CloseAll();
    // Recreate manager to reset io_context (joins io_thread cleanly)
    manager_ = std::make_unique<SessionManager>();
}

void NetworkService::send(uint8_t messageType, const std::string &jsonBody) {
    auto session = manager_->GetSession(sessionName_);
    if (!session) return;
    session->WriteRaw(static_cast<MessageType>(messageType), jsonBody);
}

bool NetworkService::isConnected() const {
    return connected_.load(std::memory_order_relaxed);
}
