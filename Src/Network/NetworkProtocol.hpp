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
 */

enum class Protocol { TCP, UDP };

enum class MessageType : uint8_t {
  LOGIN = 0x01,
  SUCCESS = 0x02,
  FAIL = 0x03,
  DEVICE = 0x04,
  AVAILABLE = 0x05,
  AI = 0x06,
  CAMERA = 0x07,
  ASSIGN = 0x08,
  META = 0x09,
  IMAGE = 0x0a,
  TLS_HANDSHAKE = 0x0b
};

#pragma pack(push, 1)
struct PacketHeader {
  MessageType type;
  uint32_t length;
};

/// UDP 전용 이미지 헤더
struct ImageHeader : PacketHeader {
  uint8_t FrameNumber;
  uint8_t SequenceNumber;
  uint8_t MaxSequenceNumber;
};
#pragma pack(pop)

/// IMAGE 메타데이터 — TCP 로 수신되는 이미지 스트림 정보
struct ImageMeta {
  int total_frames;
  int jpeg_size;
  int frame_index;
  int64_t timestamp_ms;
};

/// 페이로드 타입 에일리어스
using Payload = std::shared_ptr<std::vector<uint8_t>>;

using CameraPayload =
    std::variant<std::string, std::vector<uint8_t>,
                 std::pair<std::array<float, 4>, std::string>, ImageMeta>;

using AuthPayload = std::variant<std::string>;
