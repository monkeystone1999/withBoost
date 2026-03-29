#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <openssl/ssl.h>

/**
 * @file DtlsEngine.hpp
 * @brief DTLS 및 MediaDTLS 공용 엔진
 *
 * 표준 DTLS 세션과 미디어 전용 부분 암호화(MediaDTLS) 세션을 관리합니다.
 */

namespace DTLS {

/// 쿠키 초기화 (앱 시작 시 1회 호출 필수)
void InitCookie();

/// 서버용 컨텍스트 생성
SSL_CTX* ServerContext(const char* certfile = nullptr,
                       const char* keyfile = nullptr,
                       const char* cafile = nullptr);

/// 클라이언트용 컨텍스트 생성
SSL_CTX* ClientContext(const char* certfile = nullptr,
                       const char* keyfile = nullptr,
                       const char* cafile = nullptr);

/// DTLS 세션 관리 클래스
class Session {
public:
    Session(SSL_CTX* ctx, bool isServer = true,
            std::vector<uint8_t> peerIdent = {});
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&& other) noexcept;
    Session& operator=(Session&& other) noexcept;

    bool isValid() const { return ssl != nullptr; }
    bool isHandshakeDone() const;
    void setPeer(std::vector<uint8_t> peerIdent);

    std::vector<uint8_t> Handshake();
    std::vector<uint8_t> decrypt(const char* buffer, size_t size);
    std::vector<uint8_t> encrypt(const char* buffer, size_t size);
    std::vector<uint8_t> getHandshakeData();

private:
    void cleanup();
    std::vector<uint8_t> flushWriteBio();

    SSL* ssl = nullptr;
    BIO* readBio = nullptr;
    BIO* writeBio = nullptr;
    std::vector<uint8_t> peer;
    static constexpr int BufferSize = 4096;
};

} // namespace DTLS

namespace MediaDTLS {

/// 미디어용 클라이언트 컨텍스트 생성
SSL_CTX* ClientContext(const char* certfile = nullptr,
                       const char* keyfile = nullptr,
                       const char* cafile = nullptr);

/// 미디어용 서버 컨텍스트 생성
SSL_CTX* ServerContext(const char* certfile = nullptr,
                       const char* keyfile = nullptr,
                       const char* cafile = nullptr);

/// 미디어 전용 부분 암호화 DTLS 세션
class Session {
public:
    Session(SSL_CTX* ctx, bool isServer = true,
            std::vector<uint8_t> peerIdent = {},
            size_t encryptLimit = 128);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&& other) noexcept;
    Session& operator=(Session&& other) noexcept;

    bool isValid() const { return ssl != nullptr; }
    bool isHandshakeDone() const;
    void setPeer(std::vector<uint8_t> peerIdent);

    std::vector<uint8_t> Handshake();
    std::vector<uint8_t> decrypt(const char* buffer, size_t size);
    std::vector<uint8_t> encrypt(const char* buffer, size_t size);
    std::vector<uint8_t> getHandshakeData();

    /// 부분 암호화/복호화
    std::vector<uint8_t> PartialEncrypt(const uint8_t* data, size_t size);
    std::vector<uint8_t> PartialDecrypt(const uint8_t* data, size_t size);

private:
    void cleanup();
    std::vector<uint8_t> flushWriteBio();

    SSL* ssl = nullptr;
    BIO* readBio = nullptr;
    BIO* writeBio = nullptr;
    std::vector<uint8_t> peer;
    size_t limit_;
    static constexpr int BufferSize = 4096;
};

} // namespace MediaDTLS
