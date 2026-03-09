#pragma once
#include <functional>
#include <cstdint>
#include <string>

// ============================================================
//  INetworkService — Pure interface for TCP network operations
//
//  WHY:  NetworkBridge previously owned SessionManager directly.
//        There was no seam for testing or swapping the backend.
//        This interface lets Core.cpp inject a real or mock service.
//
//  RULE: No Qt types. No boost types visible in the interface.
//        All callbacks use std::function<void(std::string)>.
// ============================================================

struct NetworkCallbacks {
    std::function<void()>                    onConnected;
    std::function<void(const std::string &)> onLoginResult;   // SUCCESS body
    std::function<void(const std::string &)> onLoginFail;     // FAIL body
    std::function<void(const std::string &)> onCameraList;    // CAMERA body
    std::function<void(const std::string &)> onDeviceStatus;  // DEVICE body
    std::function<void(const std::string &)> onAiResult;      // AI body
    std::function<void(const std::string &)> onAssign;        // ASSIGN body
};

class INetworkService {
public:
    virtual ~INetworkService() = default;

    virtual void connect   (const std::string &host,
                            const std::string &port,
                            NetworkCallbacks   callbacks) = 0;
    virtual void disconnect() = 0;
    virtual void send      (uint8_t            messageType,
                            const std::string &jsonBody) = 0;
    virtual bool isConnected() const = 0;
};
