#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <openssl/ssl.h>

/**
 * @file TlsEngine.hpp
 * @brief TLS 세션 및 컨텍스트 관리 엔진
 */

namespace TLS {

/// 서버용 컨텍스트 생성
SSL_CTX* ServerContext(const char* certfile = nullptr,
                       const char* keyfile = nullptr,
                       const char* cafile = nullptr);

/// 클라이언트용 컨텍스트 생성
SSL_CTX* ClientContext(const char* certfile = nullptr,
                       const char* keyfile = nullptr,
                       const char* cafile = nullptr);

/// TLS 세션 관리 클래스
class Session {
public:
    Session(SSL_CTX* ctx, bool isServer = true);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&& other) noexcept;
    Session& operator=(Session&& other) noexcept;

    bool isValid() const { return ssl != nullptr; }
    bool isHandshakeDone() const;

    std::vector<uint8_t> Handshake();
    std::vector<uint8_t> decrypt(const char* buffer, int len);
    std::vector<uint8_t> encrypt(const char* buffer, int len);
    std::vector<uint8_t> getHandshakeData();

private:
    void cleanup();
    std::vector<uint8_t> flushWriteBio();

    SSL* ssl = nullptr;
    BIO* readBio = nullptr;
    BIO* writeBio = nullptr;
    static constexpr int BufferSize = 4096;
};

} // namespace TLS
