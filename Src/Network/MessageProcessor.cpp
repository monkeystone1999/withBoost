#include "MessageProcessor.hpp"
#include <iostream>
#include <string>

namespace anomap {
namespace network {

MessageProcessor::MessageProcessor(NetworkCallbacks callbacks)
    : callbacks_(callbacks) {}

void MessageProcessor::processMessage(const Message &msg,
                                      std::shared_ptr<TcpSession> session) {
  auto type = static_cast<MessageType>(msg.type);

  switch (type) {
  case MessageType::SUCCESS:
    handleLoginResult(msg);
    break;
  case MessageType::FAIL:
    handleLoginFail(msg);
    break;
  case MessageType::DEVICE:
    handleDeviceStatus(msg);
    break;
  case MessageType::AVAILABLE:
    handleAvailable(msg);
    break;
  case MessageType::AI:
    handleAiResult(msg);
    break;
  case MessageType::CAMERA:
    handleCameraList(msg);
    break;
  case MessageType::ASSIGN:
    handleAssign(msg);
    break;
  case MessageType::META:
    handleMeta(msg);
    break;
  case MessageType::IMAGE:
    handleImage(msg);
    break;
  default:
    break;
  }
}

// These just forward the JSON payload to the corresponding callback if it
// exists. We convert the payload bytes back to a string where expected.

void MessageProcessor::handleLoginResult(const Message &msg) {
  if (callbacks_.onLoginResult && !msg.payload.empty()) {
    std::string jsonStr(msg.payload.begin(), msg.payload.end());
    callbacks_.onLoginResult(jsonStr);
  }
}

void MessageProcessor::handleLoginFail(const Message &msg) {
  if (callbacks_.onLoginFail && !msg.payload.empty()) {
    std::string jsonStr(msg.payload.begin(), msg.payload.end());
    callbacks_.onLoginFail(jsonStr);
  }
}

void MessageProcessor::handleDeviceStatus(const Message &msg) {
  if (callbacks_.onDeviceStatus && !msg.payload.empty()) {
    std::string jsonStr(msg.payload.begin(), msg.payload.end());
    callbacks_.onDeviceStatus(jsonStr);
  }
}

void MessageProcessor::handleAvailable(const Message &msg) {
  // Note: The UI layer needs to intercept this and route to 'AVAILABLE' logic
  // We repurpose onDeviceStatus or user needs to add onAvailable if missing
  // from callbacks. Currently onDeviceStatus is used for some DEVICE callbacks,
  // checking if there's an onAvailable: Actually INetworkService.hpp doesn't
  // have onAvailable, it only has onDeviceStatus which might be for BOTH.
  // Error.md mentions AVAILABLE (0x05) is system state info.
  // If the frontend relies on the payload, we just pass it to onDeviceStatus
  // for now as a fallback Or add it to INetworkService.
  if (callbacks_.onDeviceStatus && !msg.payload.empty()) {
    std::string jsonStr(msg.payload.begin(), msg.payload.end());
    callbacks_.onDeviceStatus(jsonStr);
  }
}

void MessageProcessor::handleAiResult(const Message &msg) {
  if (callbacks_.onAiResult && !msg.payload.empty()) {
    std::string jsonStr(msg.payload.begin(), msg.payload.end());
    callbacks_.onAiResult(jsonStr);
  }
}

void MessageProcessor::handleCameraList(const Message &msg) {
  if (callbacks_.onCameraList && !msg.payload.empty()) {
    std::string jsonStr(msg.payload.begin(), msg.payload.end());
    callbacks_.onCameraList(jsonStr);
  }
}

void MessageProcessor::handleAssign(const Message &msg) {
  if (callbacks_.onAssign && !msg.payload.empty()) {
    std::string jsonStr(msg.payload.begin(), msg.payload.end());
    callbacks_.onAssign(jsonStr);
  }
}

void MessageProcessor::handleMeta(const Message &msg) {
  if (callbacks_.onMetaResult && !msg.payload.empty()) {
    std::string jsonStr(msg.payload.begin(), msg.payload.end());
    callbacks_.onMetaResult(jsonStr);
  }
}

void MessageProcessor::handleImage(const Message &msg) {
  if (callbacks_.onImageReceived && !msg.payload.empty()) {
    callbacks_.onImageReceived(msg.payload);
  }
}

} // namespace network
} // namespace anomap
