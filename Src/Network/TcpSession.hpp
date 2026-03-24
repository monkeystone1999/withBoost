#pragma once

#include "../Crypt/TLS.hpp"
#include "Protocol.hpp"
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
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
             DisconnectHandler discHandler, SSL_CTX *ctx = nullptr,
             bool isServer = false)
      : socket_(std::move(socket)),
        strand_(boost::asio::make_strand(socket_.get_executor())),
        messageHandler_(std::move(msgHandler)),
        disconnectHandler_(std::move(discHandler)),
        tlsSession_(ctx ? std::make_unique<TLS::Session>(ctx, isServer)
                        : nullptr) {
    headerBuffer_.resize(5);
  }

  void start() {
    if (!tlsSession_) {
      if (connectedHandler_) {
        connectedHandler_(shared_from_this());
      }
    } else {
      handleTlsOutput();
    }
    readHeader();
  }

  void setConnectedHandler(ConnectedHandler handler) {
    connectedHandler_ = std::move(handler);
  }

  void sendMessage(const Message &msg) {
    auto self = shared_from_this();

    boost::asio::post(strand_, [this, self, msg]() {
      std::vector<uint8_t> finalPayload = msg.payload;
      if (tlsSession_ && msg.length > 0) {
        finalPayload = tlsSession_->encrypt(
            reinterpret_cast<const char *>(msg.payload.data()), msg.length);
      }

      auto writeBuffer =
          std::make_shared<std::vector<uint8_t>>(5 + finalPayload.size());
      std::memcpy(writeBuffer->data(), &msg.type, 1);
      uint32_t len = static_cast<uint32_t>(finalPayload.size());
      std::memcpy(writeBuffer->data() + 1, &len, 4);
      if (len > 0) {
        std::memcpy(writeBuffer->data() + 5, finalPayload.data(), len);
      }

      boost::asio::async_write(
          socket_, boost::asio::buffer(*writeBuffer),
          boost::asio::bind_executor(
              strand_, [self, writeBuffer](boost::system::error_code ec,
                                           std::size_t /*length*/) {
                if (ec) {
                  self->handleDisconnect();
                }
              }));
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
        boost::asio::bind_executor(strand_, [this,
                                             self](boost::system::error_code ec,
                                                   std::size_t /*length*/) {
          if (!ec) {
            std::memcpy(&currentMessage_.type, headerBuffer_.data(), 1);
            std::memcpy(&currentMessage_.length, headerBuffer_.data() + 1, 4);

            if (currentMessage_.length > 0) {
              currentMessage_.payload.resize(currentMessage_.length);
              readBody();
            } else {
              currentMessage_.payload.clear();
              if (tlsSession_ &&
                  currentMessage_.type ==
                      static_cast<uint8_t>(MessageType::TLS_HANDSHAKE)) {
                handleTlsOutput();
              } else {
                if (messageHandler_)
                  messageHandler_(currentMessage_, self);
              }
              readHeader();
            }
          } else {
            handleDisconnect();
          }
        }));
  }

  void readBody() {
    auto self = shared_from_this();
    boost::asio::async_read(
        socket_, boost::asio::buffer(currentMessage_.payload),
        boost::asio::bind_executor(strand_, [this,
                                             self](boost::system::error_code ec,
                                                   std::size_t /*length*/) {
          if (!ec) {
            if (tlsSession_ && currentMessage_.length > 0) {
              currentMessage_.payload = tlsSession_->decrypt(
                  reinterpret_cast<char *>(currentMessage_.payload.data()),
                  currentMessage_.length);
              currentMessage_.length =
                  static_cast<uint32_t>(currentMessage_.payload.size());
              handleTlsOutput();
              if (currentMessage_.type ==
                  static_cast<uint8_t>(MessageType::TLS_HANDSHAKE)) {

                if (tlsSession_->isHandshakeDone() && !handshakeCompleted_) {
                  handshakeCompleted_ = true;
                  if (connectedHandler_) {
                    connectedHandler_(self);
                  }
                }

                readHeader();
                return;
              }
            }

            // --- CUSTOM IMAGE LOGIC (0x0A) ---
            if (currentMessage_.type ==
                    static_cast<uint8_t>(MessageType::IMAGE) &&
                currentMessage_.length > 0) {
              try {
                // Payload contains JSON string. We need to parse jpeg_size.
                std::string jsonStr(currentMessage_.payload.begin(),
                                    currentMessage_.payload.end());
                auto parsed = nlohmann::json::parse(jsonStr);
                int jpegSize = parsed.value("jpeg_size", 0);

                if (jpegSize > 0) {
                  // Trigger secondary read for the JPEG payload
                  // In TLS mode, we read exactly jpegSize bytes of CIPHERTEXT.
                  // Wait, ToServer.md says: "Payload는 TLS로
                  // 암호화(Ciphertext)되어 전송됩니다." The client doesn't know
                  // the exact ciphertext length if it only knows the plaintext
                  // jpegSize.
                  // ...
                  // Let's implement a secondary read into a temporary buffer.
                  auto jpegBuffer =
                      std::make_shared<std::vector<uint8_t>>(jpegSize);
                  boost::asio::async_read(
                      socket_, boost::asio::buffer(*jpegBuffer),
                      boost::asio::bind_executor(
                          strand_,
                          [this, self, jpegBuffer, jsonStr](
                              boost::system::error_code ec2, std::size_t) {
                            if (!ec2) {
                              std::vector<uint8_t> plainJpeg = *jpegBuffer;
                              if (tlsSession_) {
                                plainJpeg = tlsSession_->decrypt(
                                    reinterpret_cast<char *>(plainJpeg.data()),
                                    plainJpeg.size());
                                handleTlsOutput();
                              }

                              // Combine JSON + \0 + JPEG
                              currentMessage_.payload.clear();
                              currentMessage_.payload.insert(
                                  currentMessage_.payload.end(),
                                  jsonStr.begin(), jsonStr.end());
                              currentMessage_.payload.push_back(
                                  '\0'); // delimiter
                              currentMessage_.payload.insert(
                                  currentMessage_.payload.end(),
                                  plainJpeg.begin(), plainJpeg.end());
                              currentMessage_.length = static_cast<uint32_t>(
                                  currentMessage_.payload.size());

                              if (messageHandler_)
                                messageHandler_(currentMessage_, self);

                              readHeader();
                            } else {
                              handleDisconnect();
                            }
                          }));
                  return; // return here to wait for the secondary read
                }
              } catch (...) {
                // Invalid JSON or missing jpeg_size, ignore the rest
              }
            }
            // --- END CUSTOM IMAGE LOGIC ---

            if (messageHandler_ && currentMessage_.length > 0)
              messageHandler_(currentMessage_, self);

            readHeader();
          } else {
            handleDisconnect();
          }
        }));
  }

  void handleDisconnect() {
    if (!disconnected_) {
      disconnected_ = true;
      close();
      if (disconnectHandler_) {
        disconnectHandler_(shared_from_this());
      }
    }
  }
  void handleTlsOutput() {
    if (!tlsSession_)
      return;
    auto response = tlsSession_->Handshake();
    if (!response.empty()) {
      auto writeBuffer =
          std::make_shared<std::vector<uint8_t>>(5 + response.size());
      uint8_t type = static_cast<uint8_t>(MessageType::TLS_HANDSHAKE);
      uint32_t len = static_cast<uint32_t>(response.size());
      std::memcpy(writeBuffer->data(), &type, 1);
      std::memcpy(writeBuffer->data() + 1, &len, 4);
      std::memcpy(writeBuffer->data() + 5, response.data(), len);

      auto self = shared_from_this();
      boost::asio::post(strand_, [this, self, writeBuffer]() {
        boost::asio::async_write(
            socket_, boost::asio::buffer(*writeBuffer),
            boost::asio::bind_executor(
                strand_, [self, writeBuffer](boost::system::error_code ec,
                                             std::size_t /*length*/) {
                  if (ec) {
                    self->handleDisconnect();
                  }
                }));
      });
    }
  }

  tcp::socket socket_;
  boost::asio::strand<boost::asio::ip::tcp::socket::executor_type> strand_;
  std::vector<uint8_t> headerBuffer_;
  Message currentMessage_;
  MessageHandler messageHandler_;
  DisconnectHandler disconnectHandler_;
  ConnectedHandler connectedHandler_;
  bool disconnected_ = false;
  bool handshakeCompleted_ = false;
  std::unique_ptr<TLS::Session> tlsSession_;
};

} // namespace network
} // namespace anomap
