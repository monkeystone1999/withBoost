#pragma once
#include "INetworkService.hpp"
#include "Session.hpp"
#include <atomic>
#include <memory>

// ============================================================
//  NetworkService — INetworkService backed by SessionManager
//
//  WHEN:  Constructed once in Core::init(), before any QObject.
//  WHERE: Src/Network
//  WHY:   All SessionManager lifetime management and callback wiring
//         previously lived inside NetworkBridge (a QObject).
//         NetworkBridge must not own infrastructure — it is a type bridge.
//  HOW:   NetworkService owns SessionManager and one ConnectServer session
//         named by Config::SESSION_NAME. It translates SessionManager
//         raw callbacks into the NetworkCallbacks interface callbacks.
// ============================================================

class NetworkService : public INetworkService {
public:
    NetworkService();
    ~NetworkService() override;

    void connect   (const std::string &host,
                    const std::string &port,
                    NetworkCallbacks   callbacks) override;
    void disconnect() override;
    void send      (uint8_t            messageType,
                    const std::string &jsonBody) override;
    bool isConnected() const override;

private:
    std::unique_ptr<SessionManager> manager_;
    std::atomic<bool>               connected_{false};
    std::string                     sessionName_;
};
