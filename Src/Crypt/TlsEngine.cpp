#include "TlsEngine.hpp"
#include <iostream>

namespace TLS {

SSL_CTX* ServerContext(const char* certfile, const char* keyfile, const char* cafile) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) return nullptr;
    if (certfile && SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) != 1)
        std::cerr << "인증서 로드 실패" << std::endl;
    if (keyfile && SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1)
        std::cerr << "개인키 로드 실패" << std::endl;
    if (cafile) {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
        SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
    } else {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    }
    return ctx;
}

SSL_CTX* ClientContext(const char* certfile, const char* keyfile, const char* cafile) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
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

Session::Session(SSL_CTX* ctx, bool isServer) {
    ssl = SSL_new(ctx);
    if (!ssl) return;
    readBio = BIO_new(BIO_s_mem());
    writeBio = BIO_new(BIO_s_mem());
    BIO_set_mem_eof_return(readBio, -1);
    BIO_set_mem_eof_return(writeBio, -1);
    SSL_set_bio(ssl, readBio, writeBio);
    if (isServer) SSL_set_accept_state(ssl);
    else SSL_set_connect_state(ssl);
}

Session::~Session() { cleanup(); }

Session::Session(Session&& other) noexcept
    : ssl(other.ssl), readBio(other.readBio), writeBio(other.writeBio) {
    other.ssl = nullptr;
    other.readBio = other.writeBio = nullptr;
}

Session& Session::operator=(Session&& other) noexcept {
    if (this != &other) {
        cleanup();
        ssl = other.ssl;
        readBio = other.readBio;
        writeBio = other.writeBio;
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

std::vector<uint8_t> Session::decrypt(const char *buffer, int len) {
    std::vector<uint8_t> plainText;
    if (!ssl) return plainText;
    BIO_write(readBio, buffer, len);
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

std::vector<uint8_t> Session::encrypt(const char *buffer, int len) {
    if (!ssl || !SSL_is_init_finished(ssl)) return {};
    SSL_write(ssl, buffer, len);
    return flushWriteBio();
}

std::vector<uint8_t> Session::getHandshakeData() {
    return flushWriteBio();
}

} // namespace TLS
