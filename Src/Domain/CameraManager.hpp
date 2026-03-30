#pragma once
#include "../Network/NetworkProtocol.hpp"
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>


/**
 * @file CameraManager.hpp
 * @brief 카메라 데이터 개체 및 중앙 관리자
 *
 * 이 파일은 개별 카메라의 물리적 정보, 센서 메타데이터,
 * 그리고 조각화된 네트워크 이미지 패킷의 재조립 및 관리 로직을 정의합니다.
 */

#include "../Network/VideoEngine.hpp"

/**
 * @struct CameraMeta
 * @brief 카메라로부터 수신된 환경 센서 데이터 이력
 */
struct CameraMeta {
  std::deque<float> tmp_{20, 0.0f}, tilt_{20, 0.0f}, light_{20, 0.0f},
      hum_{20, 0.0f}; /**< 최근 20개의 데이터 포인트 (온도, 틸트, 조도, 습도) */
  std::string dir{""}; /**< 현재 장치의 방향 정보 */

  /**
   * @brief 신규 센서 데이터 추가
   * @param meta [0:온도, 1:틸트, 2:조도, 3:습도] 배열
   * @param str 방향 문자열
   */
  void Add(const std::array<float, 4> &meta, const std::string &str);
};

/** @brief 카메라 하드웨어 물리 상태 (모터, IR, 히터) */
struct CameraStatus {
  bool motor_auto{false}; /**< 모터 자동 회전 상태 */
  bool ir_on{false};      /**< IR(적외선) 라이트 상태 */
  bool heater_on{false};  /**< 내부 히터 가동 상태 */
};

/**
 * @struct CameraImage
 * @brief 네트워크를 통해 수신된 이미지 패킷 재조립 및 관리 구조체
 */
struct CameraImage {
  using IMG = std::vector<uint8_t>;
  std::deque<IMG> ImageList_; /**< 최근 디코딩된 전체 이미지 이력 */
  int jpeg_size_ = 0;         /**< 현재 수신 대기 중인 JPEG 원본 크기 */
  int total_frames_ = 0;      /**< 현재 스트림의 총 프레임 수 정보 */

  /** @brief UDP 조각 데이터 임시 보관용 슬롯 */
  struct FrameSlot {
    uint8_t maxSequence = 0; /**< 해당 프레임의 마지막 시퀀스 번호 */
    std::map<uint8_t, std::vector<uint8_t>>
        fragments; /**< 수신된 조각 데이터 맵 (Key: SequenceNumber) */
  };
  std::map<uint8_t, FrameSlot>
      pendingFrames_; /**< 재조립 중인 프레임 대기열 (Key: FrameNumber) */

  /** @brief 이미지 메타데이터(크기 등) 업데이트 */
  void SetMeta(int jpeg_size, int total_frames);
  /** @brief 완성된 바이너리 이미지 추가 */
  void Add(IMG img);
  /** @brief 원시 UDP 조각 데이터를 피딩하여 재조립 시도 */
  void Feed(const ImageHeader &header, const uint8_t *imageData,
            size_t imageSize);
  /** @brief 특정 프레임의 모든 조각이 수신되었는지 확인 */
  bool isFrameComplete(uint8_t frameNumber) const;
  /** @brief 재조립된 프레임을 추출하고 대기열에서 제거 */
  IMG ExtractFrame(uint8_t frameNumber);
  /** @brief 패킷 수신 결과(성공/손실)를 알리는 JSON 메시지 구성 */
  std::string BuildResultJson(uint8_t frameNumber) const;
  /** @brief 오래된 미완성 프레임 자원 정리 */
  void Cleanup(uint8_t currentFrame);
};

/**
 * @struct CameraInfo
 * @brief 카메라 엔터티의 모든 정보를 포함하는 통합 구조체
 */
struct CameraInfo {
  std::string rtsp_{""}, ip_{""}; /**< 접속 주소 및 식별 IP */
  uint8_t index_{0};              /**< 동일 IP 내 채널 인덱스 */
  CameraMeta MetaData_;           /**< 센서 데이터 관리 객체 */
  CameraImage Images_;            /**< 이미지 버퍼 관리 객체 */
  CameraStatus Status_;           /**< 하드웨어 상태 정보 */
  std::unique_ptr<VideoEngine>
      video_; /**< 실시간 디코딩을 위한 전용 엔진 인스턴스 */
};

/**
 * @class CameraManager
 * @brief 시스템 내 모든 카메라 엔터티의 수명 주기 및 검색 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. Add(ip, rtsp)를 호출하여 시스템에 카메라를 등록하고 내부 고유 ID를
 * 생성합니다.
 * 2. Get(), GetByIp() 등의 메서드를 통해 특정 카메라 객체의 참조를 획득합니다.
 * 3. 획득한 CameraInfo 객체의 하위 멤버(MetaData, Images)를 통해 상태를
 * 업데이트하거나 조회합니다.
 */
class CameraManager {
public:
  CameraManager() = default;
  ~CameraManager() = default;

  /**
   * @brief 신규 카메라 등록
   * @param ip 식별용 IP 주소
   * @param rtsp 실시간 스트리밍 URL
   * @return uint32_t 할당된 시스템 전역 고유 ID
   */
  uint32_t Add(std::string ip, std::string rtsp);

  /** @brief ID에 해당하는 카메라 정보 삭제 및 자원 해제 */
  void Remove(uint32_t id);

  /** @return std::map<uint32_t, CameraInfo>& 등록된 전체 카메라 맵 참조 */
  std::map<uint32_t, CameraInfo> &getCameras() { return map_; }

  /** @return CameraInfo* ID 기반 검색 결과 (없을 경우 nullptr) */
  CameraInfo *Get(uint32_t id);

  /** @return CameraInfo* 네트워크 식별자(IP/Index) 기반 검색 결과 */
  CameraInfo *Get(const std::string &networkId);

  /** @return CameraInfo* IP 주소 기반 검색 결과 (첫 번째 인스턴스) */
  CameraInfo *GetByIp(const std::string &ip);

  /** @return std::vector<uint32_t> 특정 IP에 등록된 모든 카메라 ID 리스트 */
  std::vector<uint32_t> GetIdsByIp(const std::string &ip);

private:
  std::map<uint32_t, CameraInfo> map_;     /**< ID 기반 주 저장소 */
  std::map<std::string, uint32_t> IpToId_; /**< 네트워크 식별자 기반 역인덱스 */
  std::map<std::string, std::set<uint8_t>>
      ipTable_;     /**< IP별 채널 인덱스 관리 테이블 */
  uint32_t Id_ = 0; /**< 생성할 신규 ID 카운터 */
};

/**
 * @section Workflow Guide
 *
 * **[CameraManager 데이터 관리 워크플로우]**
 *
 * 1. 데이터 업데이트 flow:
 *    - [네트워크 수신] -> [CameraBridge 식별] -> [CameraManager::Get()] ->
 * [CameraInfo::Images_.Feed()]
 *
 * 2. 조각 재조립 메커니즘:
 *    - UDP로 쪼개져 들어오는 데이터는 `CameraImage::pendingFrames_`에서
 * 관리됩니다.
 *    - 모든 시퀀스가 채워지면 `ExtractFrame`을 통해 하나의 완성된 JPEG
 * 바이너리가 되어 `ImageList_`로 이동합니다.
 *
 * 3. 자원 효율화:
 *    - `Cleanup()` 호출을 통해 장기간 보류 중인 불완전한 프레임 데이터를
 * 정기적으로 파기하여 메모리 누수를 방지하십시오.
 */
