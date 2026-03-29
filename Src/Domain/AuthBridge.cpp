#include "AuthBridge.hpp"

AuthBridge::AuthBridge(NetworkManager& networkManager, StatusManager& statusManager)
    : NetworkManager_(networkManager), StatusManager_(statusManager) {
    NetworkManager_.AuthBridge_.connect([this](std::string username, AuthPayload payload) {
        // ... Handle Auth signal from NetworkManager ...
        if (std::holds_alternative<std::string>(payload)) {
            std::string state = std::get<std::string>(payload);
            StatusManager_.AddUser(username, state);
        }
    });
}
