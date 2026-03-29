#include "NetworkEngine.hpp"
#include "NetworkManager.hpp"
#include "../Crypt/DtlsEngine.hpp"
#include <nlohmann/json.hpp>
#include <cstring>

// ─────────────────────────────────────────────────────────────
// TextConnect Implementation
// ─────────────────────────────────────────────────────────────

TextConnect::TextConnect(NetworkManager& Manager, std::string ip, uint16_t port)
    : NetworkManager_(Manager), io_(Manager.getIO()),
      TCPSession_(std::make_shared<Session<Protocol::TCP>>(Manager.getIO())) {
    TCPSession_->Connect(ip, port);
    NetworkManager_.OutboundImageBridge_.connect([this](const std::string& msg){
        this->SendImageReceiveResult(msg);
    });
}

void TextConnect::run(ThreadEngine* pool) {
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
        auto Header_ = reinterpret_cast<PacketHeader*>(Header->data());

        auto Body_ = std::make_shared<std::vector<uint8_t>>(Header_->length);
        self->TCPSession_->Read(Body_, [self, Body_, Header_]() {
            if (Header_->type == MessageType::IMAGE) {
                nlohmann::json portJson;
                portJson["port"] = self->getMediaPort();
                self->Send(MessageType::IMAGE, portJson.dump());
            }

            self->NetworkManager_.Packets(Header_->type, Body_);
            self->Read();
        });
    });
}

void TextConnect::Send(MessageType type, const std::string& jsonBody) {
    PacketHeader header;
    header.type = type;
    header.length = static_cast<uint32_t>(jsonBody.size());

    std::vector<uint8_t> packet(sizeof(PacketHeader) + jsonBody.size());
    std::memcpy(packet.data(), &header, sizeof(PacketHeader));
    std::memcpy(packet.data() + sizeof(PacketHeader), jsonBody.data(), jsonBody.size());

    TCPSession_->Write(std::move(packet));
}

void TextConnect::SendImageReceiveResult(const std::string& jsonBody) {
    Send(MessageType::IMAGE, jsonBody);
}

void TextConnect::stop() {
    TCPSession_->stop();
}

uint16_t TextConnect::getMediaPort() const {
    return MediaConnect_ ? MediaConnect_->LocalPort() : 0;
}

// ─────────────────────────────────────────────────────────────
// MediaConnect Implementation
// ─────────────────────────────────────────────────────────────

MediaConnect::MediaConnect(NetworkManager& Manager)
    : NetworkManager_(Manager), io_(Manager.getIO()),
      UDPSession_(std::make_shared<Session<Protocol::UDP>>(Manager.getIO())) {
    UDPSession_->Bind();
}

uint16_t MediaConnect::LocalPort() const {
    return UDPSession_->LocalPort();
}

void MediaConnect::run(ThreadEngine* pool) {
    if (pool == nullptr) {
        io_.run();
    } else {
        pool_ = pool;
        pool_->Submit([self = shared_from_this()]() { self->run(); });
    }
}

void MediaConnect::Read() {
    UDPSession_->StartReceive([self = shared_from_this()](auto buf, auto bytes, auto sender) {
        self->onReceive(buf, bytes, sender);
    });
}

void MediaConnect::Write(std::vector<uint8_t> data) {
    UDPSession_->Write(std::move(data));
}

void MediaConnect::stop() {
    UDPSession_->stop();
}

void MediaConnect::onReceive(std::shared_ptr<std::vector<uint8_t>> buf, std::size_t bytes, boost::asio::ip::udp::endpoint sender) {
    if (bytes < sizeof(ImageHeader)) return;

    ImageHeader header;
    std::memcpy(&header, buf->data(), sizeof(ImageHeader));

    if (header.type != MessageType::IMAGE) return;

    const uint8_t* imageData = buf->data() + sizeof(ImageHeader);
    size_t imageSize = bytes - sizeof(ImageHeader);

    // Call UDP Bridge with sender IP
    NetworkManager_.UdpImageBridge_(sender.address().to_string(), header, imageData, imageSize);
}
