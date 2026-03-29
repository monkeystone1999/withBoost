#pragma once
#include "StatusManager.hpp"
#include "../Network/NetworkManager.hpp"
#include <memory>

/**
 * @file AuthBridge.hpp
 * @brief 인증 및 사용자 권한 서비스 중계자
 */

class AuthBridge {
public:
    AuthBridge(NetworkManager& networkManager, StatusManager& statusManager);

private:
    NetworkManager& NetworkManager_;
    StatusManager& StatusManager_;
};
