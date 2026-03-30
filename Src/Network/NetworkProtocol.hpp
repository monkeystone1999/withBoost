#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

/**
 * @file NetworkProtocol.hpp
 * @brief 프로젝트 공용 네트워크 프로토콜 정의
 *
 * 이 파일은 서버와 클라이언트 간의 통신 규약을 정의합니다.
 * 모든 패킷의 공통 헤더 구조, 메시지 유형(MessageType), 그리고
 * 데이터 직렬화/역직렬화에 사용되는 데이터 구조체들을 포함합니다.
 */

/** @brief 통신 프로토콜 유형 */
enum class Protocol { TCP, UDP };

/**
 * @enum MessageType
 * @brief 패킷의 목적을 식별하는 메시지 유형 코드
 */
enum class MessageType : uint8_t {
  LOGIN = 0x01,        /**< 로그인 요청 */
  SUCCESS = 0x02,      /**< 요청 처리 성공 응답 */
  FAIL = 0x03,         /**< 요청 처리 실패 응답 */
  DEVICE = 0x04,       /**< 장치 정보 업데이트 */
  AVAILABLE = 0x05,    /**< 가용 상태 확인 */
  AI = 0x06,           /**< AI 분석 결과(로그) 데이터 */
  CAMERA = 0x07,       /**< 카메라 설정 및 연결 URL 정보 */
  ASSIGN = 0x08,       /**< 관리자 권한 부여/할당 */
  META = 0x09,         /**< 센서 데이터 (온도, 습도 등) */
  IMAGE = 0x0a,        /**< 이미지 스트림 및 썸네일 데이터 */
  TLS_HANDSHAKE = 0x0b /**< 보안 레이어 핸드셰이크 전용 데이터 */
};

#pragma pack(push, 1)
/**
 * @struct PacketHeader
 * @brief 모든 네트워크 패킷의 선두에 위치하는 고정 크기 헤더
 */
struct PacketHeader {
  MessageType type; /**< 패킷 유형 */
  uint32_t length;  /**< 헤더를 제외한 페이로드의 총 바이트 길이 */
};

/**
 * @struct ImageHeader
 * @brief UDP를 통한 이미지 데이터 전송 시 사용되는 확장 헤더
 * @note PacketHeader를 상속받으며, 조각화(Fragmentation)된 패킷의 순서를
 * 관리합니다.
 */
struct ImageHeader : PacketHeader {
  uint8_t FrameNumber;       /**< 현재 프레임의 고유 번호 */
  uint8_t SequenceNumber;    /**< 프레임 내 조각(Chunk)의 순번 (0부터 시작) */
  uint8_t MaxSequenceNumber; /**< 현재 프레임의 총 조각 수 - 1 */
};
#pragma pack(pop)

/**
 * @struct ImageMeta
 * @brief TCP 제어 채널을 통해 전달되는 이미지 스트림의 메타 정보
 */
struct ImageMeta {
  int total_frames;     /**< 전체 프레임 수 */
  int jpeg_size;        /**< JPEG 원본 데이터 크기 */
  int frame_index;      /**< 현재 프레임 인덱스 */
  int64_t timestamp_ms; /**< 생성 시간 (밀리초 단위) */
};

/** @brief 원시 바이트 데이터를 담는 공유 컨테이너 에일리어스 */
using Payload = std::shared_ptr<std::vector<uint8_t>>;

/** @brief 카메라 엔터티와 관련된 다양한 페이로드 타입을 포함하는 가변형 타입 */
using CameraPayload =
    std::variant<std::string, std::vector<uint8_t>,
                 std::pair<std::array<float, 4>, std::string>, ImageMeta>;

/** @brief 인증 서비스와 관련된 페이로드 가변형 타입 */
using AuthBridgePayload = std::variant<std::string>;

/**
 * @section Workflow Guide
 *
 * **[프로토콜 정의 및 사용 원칙]**
 *
 * 1. 바이트 정렬 (Padding):
 *    - 네트워크 전송 구조체는 반드시 `#pragma pack(push, 1)`을 사용하여 바인딩
 * 시 정렬 오버헤드를 제거해야 합니다.
 *
 * 2. 패킷 구성 가이드:
 *    - [PacketHeader (5 bytes)] + [JSON Body (N bytes)] 형식으로 구성하는 것이
 * 원칙입니다.
 *    - 단, 실시간 미디어의 경우 `ImageHeader`를 사용하며 페이로드는 원시
 * 바이너리(JPEG 등)가 될 수 있습니다.
 *
 * 3. 데이터 확장:
 *    - 메시지 타입을 추가할 때는 `MessageType` enum의 마지막 번호를 이어서
 * 사용하고, 상응하는 페이로드 구조체를 본 파일에 정의하는 것을 권장합니다.
 */
