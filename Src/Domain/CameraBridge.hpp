#pragma once
#include "../Network/NetworkManager.hpp"
#include "CameraManager.hpp"
#include <memory>


/**
 * @file CameraBridge.hpp
 * @brief 네트워크 패킷과 카메라 도메인 간의 중계자
 *
 * 이 파일은 `NetworkManager`로부터 수신된 다양한 카메라 관련 데이터(RTSP URL,
 * 메타데이터, 이미지 등)를 `CameraManager` 내의 적절한 `CameraInfo` 객체로
 * 라우팅하는 브릿지 클래스를 정의합니다.
 */

/**
 * @class CameraBridge
 * @brief 네트워크 시그널과 카메라 엔터티 간의 데이터 라우팅 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 생성 시 `CameraManager`와 `NetworkManager`를 주입받아 상호 참조 관계를
 * 형성합니다.
 * 2. 제어 채널(TCP)을 통한 JSON 메시지와 미디어 채널(UDP)을 통한 이미지
 * 조각들을 각각 구독합니다.
 * 3. 수신된 데이터의 IP 주소를 기반으로 대상을 식별하여 도메인 모델을
 * 업데이트합니다.
 */
class CameraBridge {
public:
  /**
   * @brief 카메라 브릿지 생성자
   * @param cameraManager 데이터를 반영할 카메라 관리 객체
   * @param networkManager 시그널을 제공할 네트워크 관리 객체
   */
  CameraBridge(CameraManager &cameraManager, NetworkManager &networkManager);

private:
  CameraManager &CameraManager_;   /**< 카메라 도메인 레이어 참조 */
  NetworkManager &NetworkManager_; /**< 네트워크 통신 레이어 참조 */
};

/**
 * @section Workflow Guide
 *
 * **[CameraBridge 데이터 처리 워크플로우]**
 *
 * 1. JSON 이벤트 처리:
 *    - `NetworkManager_.CameraBridge_` 시그널을 통해 RTSP 주소나 센서
 * 메타데이터가 전달됩니다.
 *    - `std::visit`과 `overloaded`를 사용하여 페이로드 타입별(RTSP, 이미지,
 * 메타 등) 처리 로직을 분기합니다.
 *
 * 2. UDP 실시간 미디어 처리:
 *    - `UdpImageBridge_` 시그널을 통해 수신된 이미지 조각들을
 * `CameraImage::Feed`로 전달합니다.
 *    - 프레임이 완성되면 `ExtractFrame`을 호출하고 네트워크 레이어에
 * ACK(`receive:true`) 신호를 보냅니다.
 *    - 패킷 손실 발생 시 손실된 시퀀스 번호를 포함한 NACK 정보를 구성하여
 * 재전송을 유도합니다.
 */
