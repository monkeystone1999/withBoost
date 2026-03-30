#include "NetworkService.hpp"
#include "../Crypt/TLS.hpp"
#include "Protocol.hpp"
#include <cstdio>
#include <iostream>
#include <nlohmann/json.hpp>

using boost::asio::ip::tcp;

NetworkService::NetworkService() : sessionName_("Main") {}

NetworkService::~NetworkService() { disconnect(); }

void NetworkService::connect(const std::string &host, const std::string &port,
                             NetworkCallbacks cbs) {
  disconnect(); // Ensure any previous connection is cleaned up

  // Wrap the callbacks to intercept onImageMeta
  NetworkCallbacks wrapped_cbs = cbs;
  wrapped_cbs.onImageMeta = [this, cbs](const std::string &jsonStr) {
    boost::asio::post(*strand_, [this, cbs, jsonStr]() {
      if (!udpSession_) {
        udpSession_ = std::make_shared<anomap::network::UdpSession>(
            *io_context_, cbs.onImageData, [this](const std::string &ctrlJson) {
              // Send control message over TCP with IMAGE type
              send(static_cast<uint8_t>(anomap::network::MessageType::IMAGE),
                   ctrlJson);
            });
        udpSession_->start();
      }

      udpSession_->setExpectedMeta(jsonStr);

      // Send the bound port to the server
      uint16_t port = udpSession_->getLocalPort();
      nlohmann::json reply;
      reply["port"] = port;
      send(static_cast<uint8_t>(anomap::network::MessageType::IMAGE),
           reply.dump());

      // Forward metadata to generic handler (so UI updates CameraCard or starts
      // displaying)
      if (cbs.onImageMeta) {
        cbs.onImageMeta(jsonStr);
      }
    });
  };

  processor_ = std::make_unique<anomap::network::MessageProcessor>(wrapped_cbs);
  io_context_ = std::make_unique<boost::asio::io_context>();
  work_guard_ = std::make_unique<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
      boost::asio::make_work_guard(*io_context_));
  strand_ = std::make_unique<
      boost::asio::strand<boost::asio::io_context::executor_type>>(
      io_context_->get_executor());

  io_thread_ = std::thread([this]() { io_context_->run(); });

  boost::asio::post(*strand_, [this, host, port, wrapped_cbs]() {
    tcp::resolver resolver(*io_context_);
    auto endpoints = resolver.resolve(host, port);

    auto shared_socket = std::make_shared<tcp::socket>(*io_context_);

    boost::asio::async_connect(
        *shared_socket, endpoints,
        [this, shared_socket, wrapped_cbs](boost::system::error_code ec,
                                           tcp::endpoint) {
          if (!ec) {
            SSL_CTX *ctx = TLS::ClientContext("Auth/client.crt",
                                              "Auth/client.key", "Auth/ca.crt");
            auto newSession = std::make_shared<anomap::network::TcpSession>(
                std::move(*shared_socket),
                [this](const anomap::network::Message &msg,
                       std::shared_ptr<anomap::network::TcpSession> s) {
                  boost::asio::post(*strand_, [this, msg, s]() {
                    if (processor_)
                      processor_->processMessage(msg, s);
                  });
                },
                [this](std::shared_ptr<anomap::network::TcpSession>) {
                  boost::asio::post(*strand_, [this]() { connected_ = false; });
                },
                ctx, false);

            if (ctx)
              SSL_CTX_free(ctx);

            newSession->setConnectedHandler(
                [this, cb = wrapped_cbs.onConnected](
                    std::shared_ptr<anomap::network::TcpSession>) {
                  boost::asio::post(*strand_, [this, cb]() {
                    connected_ = true;
                    if (cb)
                      cb();
                  });
                });

            session_ = newSession;
            session_->start();
          } else {
          }
        });
  });
}

void NetworkService::disconnect() {
  if (io_context_) {
    boost::asio::dispatch(*strand_, [this]() {
      connected_ = false;
      if (session_) {
        session_->close();
        session_.reset();
      }
      if (udpSession_) { // Add proper cleanup for UdpSession
        udpSession_.reset();
      }
      if (processor_) {
        processor_.reset();
      }
    });

    if (work_guard_) {
      work_guard_->reset();
    }
    // DO NOT stop() here. We must allow the strand to execute its final
    // commands, explicitly close the sockets, and let the io_thread gracefully
    // finish. If stop() is called early, the session destruction runs into a
    // dangling io_context memory space when the process exits.
    if (io_thread_.joinable()) {
      io_thread_.join();
    }
    // Now it is completely safe to destroy the primitives in reverse order of
    // derivation. strand_ and work_guard_ hold references to io_context's
    // executor, so they MUST be destroyed BEFORE io_context!
    session_.reset();
    udpSession_.reset();
    processor_.reset();
    strand_.reset();
    work_guard_.reset();
    io_context_.reset();
  }
}

void NetworkService::send(uint8_t messageType, const std::string &jsonBody) {
  if (!strand_)
    return;

  boost::asio::post(*strand_, [this, messageType, jsonBody]() {
    if (!session_ || !connected_)
      return;

    anomap::network::Message msg;
    msg.type = messageType;
    msg.length = static_cast<uint32_t>(jsonBody.size());
    msg.payload.assign(jsonBody.begin(), jsonBody.end());

    session_->sendMessage(msg);
  });
}

bool NetworkService::isConnected() const {
  return connected_.load(std::memory_order_relaxed);
}
