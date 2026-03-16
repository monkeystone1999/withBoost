#pragma once
#include "INetworkService.hpp"
#include "MessageProcessor.hpp"
#include "TcpSession.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <thread>

// ============================================================
//  NetworkService — INetworkService backed by SessionManager
//
//  WHEN:  Constructed once in Core::init(), before any QObject.
//  WHERE: Src/Network
//  WHY:   All SessionManager lifetime management and callback wiring
//         previously lived inside NetworkBridge (a QObject).
//         NetworkBridge must not own infrastructure — it is a type bridge.
//  HOW:   NetworkService owns SessionManager and one ConnectServer session
//         named by Config::SESSION_NAME. It translates SessionManager
//         raw callbacks into the NetworkCallbacks interface callbacks.
// ============================================================

class NetworkService : public INetworkService {
public:
  NetworkService();
  ~NetworkService() override;

  void connect(const std::string &host, const std::string &port,
               NetworkCallbacks callbacks) override;
  void disconnect() override;
  void send(uint8_t messageType, const std::string &jsonBody) override;
  bool isConnected() const override;

private:
  std::shared_ptr<anomap::network::TcpSession> session_;
  std::unique_ptr<anomap::network::MessageProcessor> processor_;
  std::unique_ptr<boost::asio::io_context> io_context_;
  std::unique_ptr<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>
      work_guard_;
  std::thread io_thread_;
  std::atomic<bool> connected_{false};
  std::string sessionName_;
};
