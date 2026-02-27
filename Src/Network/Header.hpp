#pragma once
#include <cstdint>

enum class MessageType : uint8_t {
  ACK = 0x00,       // 확인 응답
  LOGIN = 0x01,     // 로그인 요청
  SUCCESS = 0x02,   // 성공 응답 (JSON 없음)
  FAIL = 0x03,      // 실패 응답
  DEVICE = 0x04,    // 장치 제어 (추후 사용)
  AVAILABLE = 0x05, // 장치 사용 수치
  AI = 0x06,        // AI 관련 메타데이터
  CAMERA = 0x07     // 카메라 리스트
};

// 패킷 헤더 (총 5바이트: Type 1 + BodyLength 4)
#pragma pack(push, 1)
struct PacketHeader {
  MessageType type;     // 메시지 종류 (1바이트)
  uint32_t body_length; // JSON 본문 크기 (4바이트, 네트워크 바이트 오더)
};
#pragma pack(pop)
