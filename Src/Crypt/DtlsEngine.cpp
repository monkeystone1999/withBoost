#include "DtlsEngine.hpp"
#include <iostream>
#include <algorithm>
#include <openssl/hmac.h>
#include <openssl/rand.h>

namespace DTLS {

std::vector<uint8_t> SecretCookie(32);
std::once_flag cookie_flag;

void InitCookie() {
    std::call_once(cookie_flag, []() {
        if (RAND_bytes(SecretCookie.data(), SecretCookie.size()) <= 0) {
            std::cerr << "Fail to Init Cookie" << std::endl;
        }
    });
}

static int GenerateCookieWithPeer(const std::vector<uint8_t> &peer,
                                  unsigned char *cookie, unsigned int *len) {
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int resultlength;
    HMAC(EVP_sha256(), SecretCookie.data(), SecretCookie.size(), peer.data(),
         static_cast<int>(peer.size()), result, &resultlength);
    memcpy(cookie, result, resultlength);
    *len = resultlength;
    return 1;
}

static int VerifyCookieWithPeer(const std::vector<uint8_t> &peer,
                                const unsigned char *cookie, unsigned int len) {
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int resultlength;
    HMAC(EVP_sha256(), SecretCookie.data(), SecretCookie.size(), peer.data(),
         static_cast<int>(peer.size()), result, &resultlength);
    if (len == resultlength && memcmp(result, cookie, resultlength) == 0)
        return 1;
    return 0;
}

static int GenerateCookie(SSL *ssl, unsigned char *cookie, unsigned int *len) {
    std::vector<uint8_t> emptyPeer;
    return GenerateCookieWithPeer(emptyPeer, cookie, len);
}

static int VerifyCookie(SSL *ssl, const unsigned char *cookie, unsigned int len) {
    std::vector<uint8_t> emptyPeer;
    return VerifyCookieWithPeer(emptyPeer, cookie, len);
}

SSL_CTX* ServerContext(const char* certfile, const char* keyfile, const char* cafile) {
    SSL_CTX *ctx = SSL_CTX_new(DTLS_server_method());
    if (!ctx) return nullptr;
    if (certfile && SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) != 1)
        std::cerr << "인증서 로드 실패" << std::endl;
    if (keyfile && SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1)
        std::cerr << "개인키 로드 실패" << std::endl;
    if (certfile && keyfile && SSL_CTX_check_private_key(ctx) != 1)
        std::cerr << "키 검증 실패" << std::endl;
    if (cafile) {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
        SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
    } else {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    }
    SSL_CTX_set_cookie_generate_cb(ctx, GenerateCookie);
    SSL_CTX_set_cookie_verify_cb(ctx, VerifyCookie);
    SSL_CTX_set_options(ctx, SSL_OP_COOKIE_EXCHANGE);
    return ctx;
}

SSL_CTX* ClientContext(const char* certfile, const char* keyfile, const char* cafile) {
    SSL_CTX *ctx = SSL_CTX_new(DTLS_client_method());
    if (!ctx) return nullptr;
    if (certfile) SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM);
    if (keyfile) SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM);
    if (cafile) {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
        SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
    } else {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    }
    return ctx;
}

Session::Session(SSL_CTX* ctx, bool isServer, std::vector<uint8_t> peerIdent)
    : peer(std::move(peerIdent)) {
    ssl = SSL_new(ctx);
    if (!ssl) return;
    if (isServer) SSL_set_accept_state(ssl);
    else SSL_set_connect_state(ssl);
    readBio = BIO_new(BIO_s_mem());
    writeBio = BIO_new(BIO_s_mem());
    BIO_set_mem_eof_return(readBio, -1);
    BIO_set_mem_eof_return(writeBio, -1);
    SSL_set_bio(ssl, readBio, writeBio);
}

Session::~Session() { cleanup(); }

Session::Session(Session&& other) noexcept
    : ssl(other.ssl), readBio(other.readBio), writeBio(other.writeBio),
      peer(std::move(other.peer)) {
    other.ssl = nullptr;
    other.readBio = other.writeBio = nullptr;
}

Session& Session::operator=(Session&& other) noexcept {
    if (this != &other) {
        cleanup();
        ssl = other.ssl;
        readBio = other.readBio;
        writeBio = other.writeBio;
        peer = std::move(other.peer);
        other.ssl = nullptr;
        other.readBio = other.writeBio = nullptr;
    }
    return *this;
}

void Session::cleanup() {
    if (ssl) {
        SSL_free(ssl);
        ssl = nullptr;
        readBio = writeBio = nullptr;
    }
}

bool Session::isHandshakeDone() const { return ssl && SSL_is_init_finished(ssl); }
void Session::setPeer(std::vector<uint8_t> peerIdent) { peer = std::move(peerIdent); }

std::vector<uint8_t> Session::flushWriteBio() {
    std::vector<uint8_t> out;
    int pending = BIO_pending(writeBio);
    if (pending > 0) {
        out.resize(pending);
        int read = BIO_read(writeBio, out.data(), pending);
        if (read > 0) out.resize(read);
        else out.clear();
    }
    return out;
}

std::vector<uint8_t> Session::Handshake() {
    if (!ssl) return {};
    if (!SSL_is_init_finished(ssl)) {
        int res = SSL_do_handshake(ssl);
        if (res <= 0) {
            int err = SSL_get_error(ssl, res);
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
                std::cerr << "Handshake error: " << err << std::endl;
        }
    }
    return flushWriteBio();
}

std::vector<uint8_t> Session::decrypt(const char *buffer, size_t size) {
    std::vector<uint8_t> plainText;
    if (!ssl) return plainText;
    BIO_write(readBio, buffer, static_cast<int>(size));
    if (!SSL_is_init_finished(ssl)) {
        SSL_do_handshake(ssl);
        if (!SSL_is_init_finished(ssl)) return plainText;
    }
    uint8_t buf[BufferSize];
    while (true) {
        int read = SSL_read(ssl, buf, sizeof(buf));
        if (read > 0) plainText.insert(plainText.end(), buf, buf + read);
        else break;
    }
    return plainText;
}

std::vector<uint8_t> Session::encrypt(const char *buffer, size_t size) {
    if (!ssl || !SSL_is_init_finished(ssl)) return {};
    SSL_write(ssl, buffer, static_cast<int>(size));
    return flushWriteBio();
}

std::vector<uint8_t> Session::getHandshakeData() {
    return flushWriteBio();
}

} // namespace DTLS

namespace MediaDTLS {

SSL_CTX* ClientContext(const char* certfile, const char* keyfile, const char* cafile) {
    SSL_CTX *ctx = SSL_CTX_new(DTLS_client_method());
    if (!ctx) return nullptr;
    if (certfile) SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM);
    if (keyfile) SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM);
    if (cafile) {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
        SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
    } else {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    }
    return ctx;
}

SSL_CTX* ServerContext(const char* certfile, const char* keyfile, const char* cafile) {
    SSL_CTX *ctx = SSL_CTX_new(DTLS_server_method());
    if (!ctx) return nullptr;
    if (certfile && SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) != 1)
        std::cerr << "MediaDTLS: 인증서 로드 실패" << std::endl;
    if (keyfile && SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1)
        std::cerr << "MediaDTLS: 개인키 로드 실패" << std::endl;
    if (cafile) {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
        SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
    } else {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    }
    return ctx;
}

Session::Session(SSL_CTX* ctx, bool isServer, std::vector<uint8_t> peerIdent, size_t encryptLimit)
    : peer(std::move(peerIdent)), limit_(encryptLimit) {
    ssl = SSL_new(ctx);
    if (!ssl) return;
    if (isServer) SSL_set_accept_state(ssl);
    else SSL_set_connect_state(ssl);
    readBio = BIO_new(BIO_s_mem());
    writeBio = BIO_new(BIO_s_mem());
    BIO_set_mem_eof_return(readBio, -1);
    BIO_set_mem_eof_return(writeBio, -1);
    SSL_set_bio(ssl, readBio, writeBio);
}

Session::~Session() { cleanup(); }

Session::Session(Session&& other) noexcept
    : ssl(other.ssl), readBio(other.readBio), writeBio(other.writeBio),
      peer(std::move(other.peer)), limit_(other.limit_) {
    other.ssl = nullptr;
    other.readBio = other.writeBio = nullptr;
}

Session& Session::operator=(Session&& other) noexcept {
    if (this != &other) {
        cleanup();
        ssl = other.ssl;
        readBio = other.readBio;
        writeBio = other.writeBio;
        peer = std::move(other.peer);
        limit_ = other.limit_;
        other.ssl = nullptr;
        other.readBio = other.writeBio = nullptr;
    }
    return *this;
}

void Session::cleanup() {
    if (ssl) {
        SSL_free(ssl);
        ssl = nullptr;
        readBio = writeBio = nullptr;
    }
}

bool Session::isHandshakeDone() const { return ssl && SSL_is_init_finished(ssl); }
void Session::setPeer(std::vector<uint8_t> peerIdent) { peer = std::move(peerIdent); }

std::vector<uint8_t> Session::flushWriteBio() {
    std::vector<uint8_t> out;
    int pending = BIO_pending(writeBio);
    if (pending > 0) {
        out.resize(pending);
        int read = BIO_read(writeBio, out.data(), pending);
        if (read > 0) out.resize(read);
        else out.clear();
    }
    return out;
}

std::vector<uint8_t> Session::Handshake() {
    if (!ssl) return {};
    if (!SSL_is_init_finished(ssl)) {
        SSL_do_handshake(ssl);
    }
    return flushWriteBio();
}

std::vector<uint8_t> Session::decrypt(const char *buffer, size_t size) {
    std::vector<uint8_t> plainText;
    if (!ssl) return plainText;
    BIO_write(readBio, buffer, static_cast<int>(size));
    if (!SSL_is_init_finished(ssl)) {
        SSL_do_handshake(ssl);
        if (!SSL_is_init_finished(ssl)) return plainText;
    }
    uint8_t buf[BufferSize];
    while (true) {
        int read = SSL_read(ssl, buf, sizeof(buf));
        if (read > 0) plainText.insert(plainText.end(), buf, buf + read);
        else break;
    }
    return plainText;
}

std::vector<uint8_t> Session::encrypt(const char *buffer, size_t size) {
    if (!ssl || !SSL_is_init_finished(ssl)) return {};
    SSL_write(ssl, buffer, static_cast<int>(size));
    return flushWriteBio();
}

std::vector<uint8_t> Session::getHandshakeData() {
    return flushWriteBio();
}

std::vector<uint8_t> Session::PartialEncrypt(const uint8_t *data, size_t size) {
    if (!isHandshakeDone()) return {};
    size_t encLen = (std::min)(size, limit_);
    auto encPart = encrypt(reinterpret_cast<const char *>(data), encLen);
    if (encPart.empty()) return {};
    uint16_t encSize = static_cast<uint16_t>(encPart.size());
    std::vector<uint8_t> out;
    out.reserve(2 + encPart.size() + (size - encLen));
    out.push_back(static_cast<uint8_t>(encSize & 0xFF));
    out.push_back(static_cast<uint8_t>((encSize >> 8) & 0xFF));
    out.insert(out.end(), encPart.begin(), encPart.end());
    if (size > encLen) out.insert(out.end(), data + encLen, data + size);
    return out;
}

std::vector<uint8_t> Session::PartialDecrypt(const uint8_t *data, size_t size) {
    if (!isHandshakeDone() || size < 2) return {};
    uint16_t encSize = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
    if (size < 2u + encSize) return {};
    auto decPart = decrypt(reinterpret_cast<const char *>(data + 2), encSize);
    size_t tailOffset = 2u + encSize;
    size_t tailLen = size - tailOffset;
    std::vector<uint8_t> out;
    out.reserve(decPart.size() + tailLen);
    out.insert(out.end(), decPart.begin(), decPart.end());
    if (tailLen > 0) out.insert(out.end(), data + tailOffset, data + tailOffset + tailLen);
    return out;
}

} // namespace MediaDTLS
