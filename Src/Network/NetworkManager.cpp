#include "NetworkManager.hpp"
#include "../Crypt/DtlsEngine.hpp"
#include "../Crypt/TlsEngine.hpp"
#include <iostream>

NetworkManager::NetworkManager() {
    /// TLS/DTLS 컨텍스트 초기화
    DTLS::InitCookie();
    tlsCtx_       = TLS::ClientContext();
    dtlsCtx_      = DTLS::ClientContext();
    mediaDtlsCtx_ = MediaDTLS::ClientContext();
}

NetworkManager::~NetworkManager() {
    if (tlsCtx_)       SSL_CTX_free(tlsCtx_);
    if (dtlsCtx_)      SSL_CTX_free(dtlsCtx_);
    if (mediaDtlsCtx_) SSL_CTX_free(mediaDtlsCtx_);
}

void NetworkManager::Packets(MessageType Type, Payload Body) {
    try {
        auto body_ = nlohmann::json::parse(Body->begin(), Body->end());

        switch (Type) {
            case MessageType::CAMERA:
                CameraBridge_(body_.value("ip", ""), body_.value("url", ""));
                break;
            case MessageType::META:
                CameraBridge_(body_.value("ip", ""),
                    std::make_pair(
                        std::array<float, 4>{
                            body_.value("tmp", 0.0f),
                            body_.value("tilt", 0.0f),
                            body_.value("light", 0.0f),
                            body_.value("hum", 0.0f)
                        },
                        body_.value("dir", "")
                    ));
                break;
            case MessageType::AI:
                CameraBridge_(body_.value("ip", ""),
                              std::vector<uint8_t>(Body->begin(), Body->end()));
                break;
            case MessageType::IMAGE:
                CameraBridge_(body_.value("ip", ""),
                              ImageMeta{
                                  body_.value("total_frames", 0),
                                  body_.value("jpeg_size", 0),
                                  body_.value("frame_index", 0),
                                  body_.value("timestamp_ms", int64_t(0))
                              });
                break;
            case MessageType::SUCCESS:
                AuthBridge_(body_.value("username", ""),
                            body_.value("state", body_.value("message", "")));
                break;
            case MessageType::FAIL:
                AuthBridge_("", body_.value("error", "unknown error"));
                break;
            case MessageType::ASSIGN:
                AuthBridge_("", body_.dump());
                break;
            case MessageType::AVAILABLE:
            case MessageType::DEVICE:
                StatusBridge_(Body);
                break;
            default:
                break;
        }
    } catch (...) {
        /// JSON 파싱 실패 무시
    }
}
