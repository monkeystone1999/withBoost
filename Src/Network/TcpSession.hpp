#pragma once

#include "Protocol.hpp"
#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

using boost::asio::ip::tcp;

namespace anomap {
namespace network {

class TcpSession : public std::enable_shared_from_this<TcpSession> {
public:
  using MessageHandler =
      std::function<void(const Message &, std::shared_ptr<TcpSession>)>;
  using DisconnectHandler = std::function<void(std::shared_ptr<TcpSession>)>;
  using ConnectedHandler = std::function<void(std::shared_ptr<TcpSession>)>;

  TcpSession(tcp::socket socket, MessageHandler msgHandler,
             DisconnectHandler discHandler)
      : socket_(std::move(socket)), messageHandler_(std::move(msgHandler)),
        disconnectHandler_(std::move(discHandler)) {
    headerBuffer_.resize(5);
  }

  void start() {
    if (connectedHandler_) {
      connectedHandler_(shared_from_this());
    }
    readHeader();
  }

  void setConnectedHandler(ConnectedHandler handler) {
    connectedHandler_ = std::move(handler);
  }

  void sendMessage(const Message &msg) {
    auto self = shared_from_this();

    auto writeBuffer =
        std::make_shared<std::vector<uint8_t>>(5 + msg.payload.size());
    std::memcpy(writeBuffer->data(), &msg.type, 1);
    std::memcpy(writeBuffer->data() + 1, &msg.length, 4);
    if (msg.length > 0) {
      std::memcpy(writeBuffer->data() + 5, msg.payload.data(), msg.length);
    }

    boost::asio::async_write(socket_, boost::asio::buffer(*writeBuffer),
                             [self, writeBuffer](boost::system::error_code ec,
                                                 std::size_t /*length*/) {
                               if (ec) {
                                 std::cerr << "[Network] Write error: "
                                           << ec.message() << "\n";
                                 self->handleDisconnect();
                               }
                             });
  }

  void close() {
    boost::system::error_code ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
  }

private:
  void readHeader() {
    auto self = shared_from_this();
    boost::asio::async_read(
        socket_, boost::asio::buffer(headerBuffer_),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            std::memcpy(&currentMessage_.type, headerBuffer_.data(), 1);
            std::memcpy(&currentMessage_.length, headerBuffer_.data() + 1, 4);

            if (currentMessage_.length > 0) {
              currentMessage_.payload.resize(currentMessage_.length);
              readBody();
            } else {
              currentMessage_.payload.clear();
              if (messageHandler_)
                messageHandler_(currentMessage_, self);
              readHeader();
            }
          } else {
            handleDisconnect();
          }
        });
  }

  void readBody() {
    auto self = shared_from_this();
    boost::asio::async_read(
        socket_, boost::asio::buffer(currentMessage_.payload),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            if (messageHandler_)
              messageHandler_(currentMessage_, self);
            readHeader();
          } else {
            handleDisconnect();
          }
        });
  }

  void handleDisconnect() {
    if (!disconnected_) {
      disconnected_ = true;
      std::cerr << "[Network] Client disconnected.\n";
      close();
      if (disconnectHandler_) {
        disconnectHandler_(shared_from_this());
      }
    }
  }

  tcp::socket socket_;
  std::vector<uint8_t> headerBuffer_;
  Message currentMessage_;
  MessageHandler messageHandler_;
  DisconnectHandler disconnectHandler_;
  ConnectedHandler connectedHandler_;
  bool disconnected_ = false;
};

} // namespace network
} // namespace anomap
