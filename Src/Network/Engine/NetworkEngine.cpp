#include "NetworkEngine.hpp"
#include "../Crypt/DtlsEngine.hpp"
#include "NetworkManager.hpp"
#include <cstring>
#include <nlohmann/json.hpp>

// ─────────────────────────────────────────────────────────────
// TextConnect Implementation
// ─────────────────────────────────────────────────────────────

TextConnect::TextConnect(NetworkManager &Manager, std::string ip, uint16_t port)
    : NetworkManager_(Manager), io_(Manager.getIO()), ip_(ip), port_(port),
      TCPSession_(std::make_shared<Session<Protocol::TCP>>(Manager.getIO())) {
  NetworkManager_.OutboundImageBridge_.connect(
      [this](const std::string &msg) { this->SendImageReceiveResult(msg); });

  // Initialize TLS Session as Client
  SSL_CTX *ctx = TLS::ClientContext();
  tlsSession_ = std::make_unique<TLS::Session>(ctx, false);
}

void TextConnect::connect() {
  TCPSession_->Connect(ip_, port_);
  // Start handshake immediately after TCP connection
  startHandshake();
}

void TextConnect::startHandshake() {
  if (handshakeInitiated_)
    return;
  handshakeInitiated_ = true;

  auto handshakeData = tlsSession_->Handshake();
  if (!handshakeData.empty()) {
    PacketHeader header;
    header.type = MessageType::TLS_HANDSHAKE;
    header.length = static_cast<uint32_t>(handshakeData.size());

    std::vector<uint8_t> packet(sizeof(PacketHeader) + handshakeData.size());
    std::memcpy(packet.data(), &header, sizeof(PacketHeader));
    std::memcpy(packet.data() + sizeof(PacketHeader), handshakeData.data(),
                handshakeData.size());

    // Atomic write to avoid packet splitting at the 5-byte boundary
    TCPSession_->Write(std::move(packet));
  }
}

void TextConnect::run(ThreadEngine *pool) {
  if (pool == nullptr) {
    io_.run();
  } else {
    pool_ = pool;
    pool_->Submit([self = shared_from_this()]() { self->run(); });
  }
}

void TextConnect::setMediaConnect(std::shared_ptr<MediaConnect> media) {
  MediaConnect_ = std::move(media);
}

void TextConnect::Read() {
  auto Header = std::make_shared<std::vector<uint8_t>>(sizeof(PacketHeader));
  TCPSession_->Read(Header, [Header, self = shared_from_this()]() {
    auto Header_ = reinterpret_cast<PacketHeader *>(Header->data());

    auto Body_ = std::make_shared<std::vector<uint8_t>>(Header_->length);
    self->TCPSession_->Read(Body_, [self, Body_, Header_]() {
      // Handle TLS Handshake packet
      if (Header_->type == MessageType::TLS_HANDSHAKE) {
        auto response = self->tlsSession_->decrypt(
            reinterpret_cast<const char *>(Body_->data()), Body_->size());

        // If TlsEngine generated response data (e.g. Server Hello response),
        // send it
        auto handshakeOut = self->tlsSession_->getHandshakeData();
        if (!handshakeOut.empty()) {
          self->Send(MessageType::TLS_HANDSHAKE,
                     std::string(handshakeOut.begin(), handshakeOut.end()));
        }

        if (self->tlsSession_->isHandshakeDone()) {
          // Handshake complete! Now we can send Login
          self->sendLogin();
        }
        self->Read();
        return;
      }

      // Decrypt normal packets if handshake is done
      std::shared_ptr<std::vector<uint8_t>> decryptedBody = Body_;
      if (self->tlsSession_->isHandshakeDone()) {
        auto plainText = self->tlsSession_->decrypt(
            reinterpret_cast<const char *>(Body_->data()), Body_->size());
        decryptedBody = std::make_shared<std::vector<uint8_t>>(
            plainText.begin(), plainText.end());
      }

      if (Header_->type == MessageType::IMAGE) {
        nlohmann::json portJson;
        portJson["port"] = self->getMediaPort();
        self->Send(MessageType::IMAGE, portJson.dump());
      }

      self->NetworkManager_.Packets(Header_->type, decryptedBody);
      self->Read();
    });
  });
}

void TextConnect::Send(MessageType type, const std::string &jsonBody) {
  if (type != MessageType::TLS_HANDSHAKE && !tlsSession_->isHandshakeDone()) {
    // Queue non-handshake messages until TLS is ready
    pendingMessages_.push_back({type, jsonBody});
    return;
  }

  std::vector<uint8_t> bodyToSend;
  if (type != MessageType::TLS_HANDSHAKE && tlsSession_->isHandshakeDone()) {
    // Encrypt body for non-handshake packets after handshake is complete
    auto encrypted = tlsSession_->encrypt(jsonBody.data(), jsonBody.size());
    bodyToSend = std::move(encrypted);
  } else {
    // Handshake or unencrypted (before handshake done)
    bodyToSend.assign(jsonBody.begin(), jsonBody.end());
  }

  PacketHeader header;
  header.type = type;
  header.length = static_cast<uint32_t>(bodyToSend.size());

  std::vector<uint8_t> packet(sizeof(PacketHeader) + bodyToSend.size());
  std::memcpy(packet.data(), &header, sizeof(PacketHeader));
  std::memcpy(packet.data() + sizeof(PacketHeader), bodyToSend.data(),
              bodyToSend.size());

  TCPSession_->Write(std::move(packet));
}

void TextConnect::sendLogin() {
  // Flush all pending messages (like LOGIN) now that TLS is ready
  while (!pendingMessages_.empty()) {
    auto msg = pendingMessages_.front();
    pendingMessages_.pop_front();
    Send(msg.type, msg.jsonBody);
  }
}

void TextConnect::SendImageReceiveResult(const std::string &jsonBody) {
  Send(MessageType::IMAGE, jsonBody);
}

void TextConnect::stop() {
  TCPSession_->stop();
  if (MediaConnect_) {
    MediaConnect_->stop();
  }
}

uint16_t TextConnect::getMediaPort() const {
  return MediaConnect_ ? MediaConnect_->LocalPort() : 0;
}

// ─────────────────────────────────────────────────────────────
// MediaConnect Implementation
// ─────────────────────────────────────────────────────────────

MediaConnect::MediaConnect(NetworkManager &Manager)
    : NetworkManager_(Manager), io_(Manager.getIO()),
      UDPSession_(std::make_shared<Session<Protocol::UDP>>(Manager.getIO())) {
  UDPSession_->Bind();
}

uint16_t MediaConnect::LocalPort() const { return UDPSession_->LocalPort(); }

void MediaConnect::run(ThreadEngine *pool) {
  if (pool == nullptr) {
    io_.run();
  } else {
    pool_ = pool;
    pool_->Submit([self = shared_from_this()]() { self->run(); });
  }
}

void MediaConnect::Read() {
  UDPSession_->StartReceive(
      [self = shared_from_this()](auto buf, auto bytes, auto sender) {
        self->onReceive(buf, bytes, sender);
      });
}

void MediaConnect::Write(std::vector<uint8_t> data) {
  UDPSession_->Write(std::move(data));
}

void MediaConnect::stop() { UDPSession_->stop(); }

void MediaConnect::onReceive(std::shared_ptr<std::vector<uint8_t>> buf,
                             std::size_t bytes,
                             boost::asio::ip::udp::endpoint sender) {
  if (bytes < sizeof(ImageHeader))
    return;

  ImageHeader header;
  std::memcpy(&header, buf->data(), sizeof(ImageHeader));

  if (header.type != MessageType::IMAGE)
    return;

  const uint8_t *imageData = buf->data() + sizeof(ImageHeader);
  size_t imageSize = bytes - sizeof(ImageHeader);

  // Call UDP Bridge with sender IP
  NetworkManager_.UdpImageBridge_(sender.address().to_string(), header,
                                  imageData, imageSize);
}

/**
 * @section Workflow Guide
 *
 * **[NetworkEngine 구현 세부 사항]**
 *
 * 1. 세션 라이프사이클:
 *    - 모든 세션은 `std::shared_from_this`를 통해 비동기 핸들러 내에서 자신의
 * 유효성을 보장합니다.
 *    - `stop()` 호출 시 소켓이 닫히며, 이후 발생하는 모든 비동기 작업은 에러
 * 코드와 함께 즉시 종료됩니다.
 *
 * 2. 스레드 안전성:
 *    - `boost::asio::strand`를 사용하여 동일 세션 내의 I/O 작업이 경합 없이
 * 순차적으로 실행되도록 보장합니다.
 *    - `NetworkManager`와의 상호작용은 주로 콜백을 통해 이루어지며, 스레드
 * 풀(`ThreadEngine`) 활용 시 스레드 간 데이터 동기화에 유의해야 합니다.
 *
 * 3. 패킷 처리 flow:
 *    - TCP (`TextConnect`): [헤더 수신] -> [길이만큼 바디 수신] -> [JSON 파싱
 * 및 비즈니스 로직 분기].
 *    - UDP (`MediaConnect`): [원시 데이터 수신] -> [이미지 헤더 검사] ->
 * [브릿지를 통한 상위 전달].
 */
