#pragma once

#include "../Crypt/MediaDTLS.hpp"
#include "Protocol.hpp"
#include <array>
#include <boost/asio.hpp>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace anomap {
namespace network {

class UdpSession : public std::enable_shared_from_this<UdpSession> {
public:
  using ImageCompleteCallback =
      std::function<void(const std::vector<uint8_t> &, const std::string &)>;

  using SendControlCallback = std::function<void(const std::string &)>;

  UdpSession(boost::asio::io_context &io_context,
             ImageCompleteCallback onComplete, SendControlCallback sendControl)
      : socket_(io_context,
                boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)),
        strand_(boost::asio::make_strand(io_context)),
        reassembly_timer_(io_context), onComplete_(std::move(onComplete)),
        sendControl_(std::move(sendControl)) {

    // Default to client mode, expecting Server to be DTLS Server.
    // However, if the server initiates, we might need ServerContext.
    // For now, we will act as DTLS Client and initiate the handshake when we
    // know the server endpoint. Actually, since UDP is connectionless, if the
    // server sends data to our port, we can capture the server endpoint and use
    // it to send data back.
    SSL_CTX *ctx = MediaDTLS::ClientContext("Auth/client.crt",
                                            "Auth/client.key", "Auth/ca.crt");
    dtlsSession_ = std::make_unique<MediaDTLS::Session>(ctx, false);
    if (ctx)
      SSL_CTX_free(ctx);
  }

  uint16_t getLocalPort() const { return socket_.local_endpoint().port(); }

  void setExpectedMeta(const std::string &meta) { expected_meta_ = meta; }

  void start() { doReceive(); }

  void setServerEndpoint(const boost::asio::ip::udp::endpoint &ep) {
    server_endpoint_ = ep;
    // Send DTLS ClientHello
    auto hello = dtlsSession_->Handshake();
    if (!hello.empty()) {
      sendRaw(hello);
    }
  }

  void sendRaw(const std::vector<uint8_t> &data) {
    if (data.empty())
      return;
    auto buf = std::make_shared<std::vector<uint8_t>>(data);
    socket_.async_send_to(
        boost::asio::buffer(*buf), server_endpoint_,
        boost::asio::bind_executor(
            strand_, [buf, ep = server_endpoint_](boost::system::error_code ec,
                                                  std::size_t bytes_sent) {
              if (ec) {
                std::cerr << "[UdpSession] Send Error: " << ec.message()
                          << " to " << ep << "\n";
              }
            }));
  }

  void logHex(const std::string &tag, const uint8_t *data, size_t len) {
    std::cerr << tag << " [" << len << " bytes]: ";
    for (size_t i = 0; i < std::min(len, size_t(16)); ++i) {
      fprintf(stderr, "%02X ", data[i]);
    }
    if (len > 16)
      std::cerr << "...";
    std::cerr << "\n";
  }

private:
  void doReceive() {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), sender_endpoint_,
        boost::asio::bind_executor(strand_, [this, self = shared_from_this()](
                                                boost::system::error_code ec,
                                                std::size_t bytes_recvd) {
          if (!ec && bytes_recvd > 0) {
            // If server endpoint isn't set, set it now.
            if (server_endpoint_.port() == 0) {
              server_endpoint_ = sender_endpoint_;
            }

            handlePacket(bytes_recvd);
          }
          if (!ec) {
            doReceive();
          }
        }));
  }

  void handlePacket(std::size_t length) {
    if (length < sizeof(ImageHeader)) {
      auto *type_ptr = reinterpret_cast<uint8_t *>(recv_buffer_.data());
      if (*type_ptr == static_cast<uint8_t>(MessageType::TLS_HANDSHAKE)) {
        std::cout << "[UdpSession] Recv Raw TLS Handshake (" << length
                  << " bytes)\n";
        auto response = dtlsSession_->Handshake(
            {recv_buffer_.begin(), recv_buffer_.begin() + length});
        if (!response.empty()) {
          sendRaw(response);
        }
        return;
      } else {
        std::cerr << "[UdpSession] Unknown/Too small packet (" << length
                  << " bytes). ";
        logHex("Data", recv_buffer_.data(), length);
        return;
      }
    }

    auto *header = reinterpret_cast<ImageHeader *>(recv_buffer_.data());
    if (header->type == MessageType::IMAGE ||
        header->type == MessageType::TLS_HANDSHAKE) {
      if (header->type == MessageType::TLS_HANDSHAKE) {
        std::cout << "[UdpSession] Recv Wrapped TLS Handshake\n";
        auto response = dtlsSession_->Handshake(
            {recv_buffer_.begin() + sizeof(PacketHeader),
             recv_buffer_.begin() + length});
        if (!response.empty()) {
          std::vector<uint8_t> out(sizeof(PacketHeader) + response.size());
          PacketHeader *out_hdr = reinterpret_cast<PacketHeader *>(out.data());
          out_hdr->type = MessageType::TLS_HANDSHAKE;
          out_hdr->length = static_cast<uint32_t>(response.size());
          std::memcpy(out.data() + sizeof(PacketHeader), response.data(),
                      response.size());
          sendRaw(out);
        }
        return;
      }

      if (header->type == MessageType::IMAGE) {
        if (!dtlsSession_->isHandshakeDone()) {
          std::cerr << "[UdpSession] Dropping IMAGE packet: DTLS Handshake not "
                       "finished.\n";
          return;
        }
        std::vector<uint8_t> plain = dtlsSession_->decrypt(
            reinterpret_cast<const char *>(recv_buffer_.data() +
                                           sizeof(ImageHeader)),
            static_cast<int>(length - sizeof(ImageHeader)));

        if (!plain.empty()) {
          handleImageFragment(header->FrameNumber, header->SequenceNumber,
                              header->MaxSequenceNumber, plain);
        } else {
          std::cerr << "[UdpSession] DTLS decrypt returned empty payload for "
                       "IMAGE frame No: "
                    << (int)header->FrameNumber
                    << " Seq: " << (int)header->SequenceNumber << "\n";
        }
      }
    } else {
      std::cerr
          << "[UdpSession] Received non-image packet with unexpected type: "
          << (int)header->type << "\n";
    }
  }

  void handleImageFragment(uint8_t frameNo, uint8_t seqNo, uint8_t maxSeqNo,
                           const std::vector<uint8_t> &data) {
    if (frameNo != current_frame_) {
      std::cout << "[UdpSession] New Frame Reassembly Start: No."
                << (int)frameNo << " (Expected fragments: " << (int)maxSeqNo + 1
                << ")\n";
      current_frame_ = frameNo;
      expected_max_seq_ = maxSeqNo;
      fragments_.clear();

      reassembly_timer_.expires_after(std::chrono::milliseconds(50));
      reassembly_timer_.async_wait(boost::asio::bind_executor(
          strand_, [this, frameNo,
                    self = shared_from_this()](boost::system::error_code ec) {
            if (!ec && current_frame_ == frameNo && !fragments_.empty()) {
              std::cerr << "[UdpSession] Reassembly Timeout for Frame "
                        << (int)frameNo << "\n";
              checkMissingFragments();
            }
          }));
    }

    if (fragments_.count(seqNo))
      return;
    fragments_[seqNo] = data;

    if (fragments_.size() == static_cast<size_t>(maxSeqNo) + 1) {
      reassembly_timer_.cancel();
      std::cout << "[UdpSession] Frame " << (int)frameNo
                << " Reassembled Successfully.\n";

      std::vector<uint8_t> complete_image;
      for (int i = 0; i <= maxSeqNo; ++i) {
        complete_image.insert(complete_image.end(), fragments_[i].begin(),
                              fragments_[i].end());
      }

      if (onComplete_)
        onComplete_(complete_image, expected_meta_);
      if (sendControl_) {
        std::cout << "[UdpSession] Sending ACK (receive:true) for Frame "
                  << (int)frameNo << "\n";
        sendControl_(R"({"receive": true})");
      }
      fragments_.clear();
    }
  }

  void checkMissingFragments() {
    std::vector<uint8_t> missing;
    for (uint8_t i = 0; i <= expected_max_seq_; ++i) {
      if (fragments_.find(i) == fragments_.end()) {
        missing.push_back(i);
      }
    }

    if (!missing.empty() && sendControl_) {
      std::cerr << "[UdpSession] Requesting retransmission for "
                << missing.size() << " fragments of Frame "
                << (int)current_frame_ << "\n";
      std::string json = "{\"frame_number\":" + std::to_string(current_frame_) +
                         ",\"missing_sequence\":[";
      for (size_t i = 0; i < missing.size(); ++i) {
        json += std::to_string(missing[i]);
        if (i < missing.size() - 1)
          json += ",";
      }
      json += "]}";
      sendControl_(json);

      reassembly_timer_.expires_after(std::chrono::milliseconds(50));
      reassembly_timer_.async_wait(boost::asio::bind_executor(
          strand_, [this, frameNo = current_frame_,
                    self = shared_from_this()](boost::system::error_code ec) {
            if (!ec && current_frame_ == frameNo && !fragments_.empty()) {
              checkMissingFragments();
            }
          }));
    }
  }

  boost::asio::ip::udp::socket socket_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::asio::steady_timer reassembly_timer_;
  boost::asio::ip::udp::endpoint sender_endpoint_;
  boost::asio::ip::udp::endpoint server_endpoint_;

  std::array<uint8_t, 2048> recv_buffer_;
  std::unique_ptr<MediaDTLS::Session> dtlsSession_;

  ImageCompleteCallback onComplete_;
  SendControlCallback sendControl_;

  std::string expected_meta_;
  uint8_t current_frame_ = 255;
  uint8_t expected_max_seq_ = 0;
  std::map<uint8_t, std::vector<uint8_t>> fragments_;
};

} // namespace network
} // namespace anomap
