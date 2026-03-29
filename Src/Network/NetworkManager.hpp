#pragma once
#include "NetworkProtocol.hpp"
#include <boost/asio.hpp>
#include <sigslot/signal.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <openssl/ssl.h>
/**
 * @file NetworkManager.hpp
 * @brief 네트워크 중앙 관리 및 패킷 디스패처
 */

class NetworkManager {
public:
    sigslot::signal<std::string, CameraPayload> CameraBridge_;
    sigslot::signal<std::string, AuthPayload> AuthBridge_;
    sigslot::signal<Payload> StatusBridge_;

    /// UDP 및 발신 시그널
    sigslot::signal<const std::string&> OutboundImageBridge_;
    sigslot::signal<const std::string&, const ImageHeader&, const uint8_t*, size_t> UdpImageBridge_;

    NetworkManager();
    ~NetworkManager();

    /// 패킷 디스패치
    void Packets(MessageType Type, Payload Body);

    boost::asio::io_context& getIO() { return io_; }
    SSL_CTX* getTlsCtx() const { return tlsCtx_; }
    SSL_CTX* getDtlsCtx() const { return dtlsCtx_; }
    SSL_CTX* getMediaDtlsCtx() const { return mediaDtlsCtx_; }

private:
    boost::asio::io_context io_;
    SSL_CTX* tlsCtx_ = nullptr;
    SSL_CTX* dtlsCtx_ = nullptr;
    SSL_CTX* mediaDtlsCtx_ = nullptr;
};
