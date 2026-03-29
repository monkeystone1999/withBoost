#pragma once
#include "CameraManager.hpp"
#include "../Network/NetworkManager.hpp"
#include <memory>

/**
 * @file CameraBridge.hpp
 * @brief 네트워크 패킷과 카메라 도메인 간의 중계자
 */

class CameraBridge {
public:
    CameraBridge(CameraManager& cameraManager,
                 NetworkManager& networkManager);

private:
    CameraManager& CameraManager_;
    NetworkManager& NetworkManager_;
};
