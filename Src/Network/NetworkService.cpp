#include "NetworkService.hpp"
#include "Protocol.hpp"
#include <cstdio>
#include <iostream>

using boost::asio::ip::tcp;

NetworkService::NetworkService() : sessionName_("Main") {}

NetworkService::~NetworkService() { disconnect(); }

void NetworkService::connect(const std::string &host, const std::string &port,
                             NetworkCallbacks cbs) {
  disconnect(); // Ensure any previous connection is cleaned up

  processor_ = std::make_unique<anomap::network::MessageProcessor>(cbs);
  io_context_ = std::make_unique<boost::asio::io_context>();
  work_guard_ = std::make_unique<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
      boost::asio::make_work_guard(*io_context_));

  tcp::resolver resolver(*io_context_);
  auto endpoints = resolver.resolve(host, port);

  tcp::socket socket(*io_context_);

  // We need to async_connect so we don't block. Wait, this architecture needs
  // the socket established. Since connect() might be called from UI thread,
  // we should async_connect.

  auto session = std::make_shared<anomap::network::TcpSession>(
      std::move(socket),
      [this](const anomap::network::Message &msg,
             std::shared_ptr<anomap::network::TcpSession> s) {
        if (processor_)
          processor_->processMessage(msg, s);
      },
      [this](std::shared_ptr<anomap::network::TcpSession>) {
        connected_ = false;
      });

  session->setConnectedHandler(
      [this,
       cb = cbs.onConnected](std::shared_ptr<anomap::network::TcpSession>) {
        connected_ = true;
        if (cb)
          cb();
      });

  session_ = session;

  // We can't move socket into session and then async_connect it.
  // We must connect the session's internal socket.
  // Wait, let's fix this block. It's better to capture session and endpoints.

  // Actually, TcpSession doesn't expose its socket. Let's create the socket,
  // async_connect it, THEN create the session. So we need a temporary
  // shared_ptr for the socket.

  auto shared_socket = std::make_shared<tcp::socket>(*io_context_);

  boost::asio::async_connect(
      *shared_socket, endpoints,
      [this, shared_socket, cbs](boost::system::error_code ec, tcp::endpoint) {
        if (!ec) {
          // Now create the session
          auto newSession = std::make_shared<anomap::network::TcpSession>(
              std::move(*shared_socket),
              [this](const anomap::network::Message &msg,
                     std::shared_ptr<anomap::network::TcpSession> s) {
                if (processor_)
                  processor_->processMessage(msg, s);
              },
              [this](std::shared_ptr<anomap::network::TcpSession>) {
                connected_ = false;
              });

          newSession->setConnectedHandler(
              [this, cb = cbs.onConnected](
                  std::shared_ptr<anomap::network::TcpSession>) {
                connected_ = true;
                if (cb)
                  cb();
              });

          session_ = newSession;
          session_->start();
        } else {
          std::cerr << "[NetworkService] Connect failed: " << ec.message()
                    << "\n";
        }
      });

  io_thread_ = std::thread([this]() { io_context_->run(); });
}

void NetworkService::disconnect() {
  connected_ = false;
  if (session_) {
    session_->close();
    session_.reset();
  }
  if (work_guard_) {
    work_guard_->reset();
  }
  if (io_context_) {
    io_context_->stop();
  }
  if (io_thread_.joinable()) {
    io_thread_.join();
  }
  io_context_.reset();
  work_guard_.reset();
  processor_.reset();
}

void NetworkService::send(uint8_t messageType, const std::string &jsonBody) {
  if (!session_ || !connected_)
    return;

  anomap::network::Message msg;
  msg.type = messageType;
  msg.length = static_cast<uint32_t>(jsonBody.size());
  msg.payload.assign(jsonBody.begin(), jsonBody.end());

  session_->sendMessage(msg);
}

bool NetworkService::isConnected() const {
  return connected_.load(std::memory_order_relaxed);
}
