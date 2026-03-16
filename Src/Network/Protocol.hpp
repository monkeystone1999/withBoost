#pragma once

#include <cstdint>
#include <vector>

namespace anomap {
namespace network {

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
  IMAGE = 0x0a
};

struct Message {
  uint8_t type;
  uint32_t length;
  std::vector<uint8_t> payload;
};

#pragma pack(push, 1)
struct PacketHeader {
  uint8_t type;
  uint32_t length;
};
#pragma pack(pop)

} // namespace network
} // namespace anomap
