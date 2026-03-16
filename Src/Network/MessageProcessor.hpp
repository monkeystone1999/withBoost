#pragma once

#include "INetworkService.hpp"
#include "TcpSession.hpp"
#include <memory>


namespace anomap {
namespace network {

class MessageProcessor {
public:
  MessageProcessor(NetworkCallbacks callbacks);
  void processMessage(const Message &msg, std::shared_ptr<TcpSession> session);

private:
  NetworkCallbacks callbacks_;

  void handleLoginResult(const Message &msg);
  void handleLoginFail(const Message &msg);
  void handleDeviceStatus(const Message &msg);
  void handleAvailable(const Message &msg);
  void handleAiResult(const Message &msg);
  void handleCameraList(const Message &msg);
  void handleAssign(const Message &msg);
  void handleMeta(const Message &msg);
  void handleImage(const Message &msg);
};

} // namespace network
} // namespace anomap
