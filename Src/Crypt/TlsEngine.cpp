#include "TlsEngine.hpp"
#include <iostream>

/**
 * @file TlsEngine.cpp
 * @brief TlsEngine 구현체 및 세션 관리 로직
 */

namespace TLS {

SSL_CTX *ServerContext(const char *certfile, const char *keyfile,
                       const char *cafile) {
  SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
  if (!ctx)
    return nullptr;
  if (certfile &&
      SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) != 1)
    std::cerr << "인증서 로드 실패" << std::endl;
  if (keyfile &&
      SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1)
    std::cerr << "개인키 로드 실패" << std::endl;
  if (cafile) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       nullptr);
    SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
  }
  return ctx;
}

SSL_CTX *ClientContext(const char *certfile, const char *keyfile,
                       const char *cafile) {
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx)
    return nullptr;
  if (certfile)
    SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM);
  if (keyfile)
    SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM);
  if (cafile) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
  }
  return ctx;
}

Session::Session(SSL_CTX *ctx, bool isServer) {
  ssl = SSL_new(ctx);
  if (!ssl)
    return;
  readBio = BIO_new(BIO_s_mem());
  writeBio = BIO_new(BIO_s_mem());
  BIO_set_mem_eof_return(readBio, -1);
  BIO_set_mem_eof_return(writeBio, -1);
  SSL_set_bio(ssl, readBio, writeBio);
  if (isServer)
    SSL_set_accept_state(ssl);
  else
    SSL_set_connect_state(ssl);
}

Session::~Session() { cleanup(); }

Session::Session(Session &&other) noexcept
    : ssl(other.ssl), readBio(other.readBio), writeBio(other.writeBio) {
  other.ssl = nullptr;
  other.readBio = other.writeBio = nullptr;
}

Session &Session::operator=(Session &&other) noexcept {
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

bool Session::isHandshakeDone() const {
  return ssl && SSL_is_init_finished(ssl);
}

std::vector<uint8_t> Session::flushWriteBio() {
  std::vector<uint8_t> out;
  char buf[BufferSize];
  while (true) {
    int read = BIO_read(writeBio, buf, sizeof(buf));
    if (read > 0) {
      out.insert(out.end(), reinterpret_cast<uint8_t *>(buf),
                 reinterpret_cast<uint8_t *>(buf) + read);
    } else {
      break;
    }
  }
  return out;
}

std::vector<uint8_t> Session::Handshake() {
  if (!ssl)
    return {};
  if (!SSL_is_init_finished(ssl)) {
    SSL_do_handshake(ssl);
  }
  return flushWriteBio();
}

std::vector<uint8_t> Session::decrypt(const char *buffer, int len) {
  std::vector<uint8_t> plainText;
  if (!ssl)
    return plainText;
  BIO_write(readBio, buffer, len);
  if (!SSL_is_init_finished(ssl)) {
    SSL_do_handshake(ssl);
    if (!SSL_is_init_finished(ssl))
      return plainText;
  }
  uint8_t buf[BufferSize];
  while (true) {
    int read = SSL_read(ssl, buf, sizeof(buf));
    if (read > 0)
      plainText.insert(plainText.end(), buf, buf + read);
    else
      break;
  }
  return plainText;
}

std::vector<uint8_t> Session::encrypt(const char *buffer, int len) {
  if (!ssl || !SSL_is_init_finished(ssl))
    return {};
  SSL_write(ssl, buffer, len);
  return flushWriteBio();
}

std::vector<uint8_t> Session::getHandshakeData() { return flushWriteBio(); }

} // namespace TLS

/**
 * @section Workflow Guide
 *
 * **[TlsEngine 구현 상세 가이드]**
 *
 * 1. 상태 전이 모델:
 *    - `decrypt()` 함수는 수신된 암호문을 `readBio`에 쓰고 `SSL_read`를
 * 시도합니다.
 *    - 만약 아직 핸드셰이크 중이라면 `SSL_read`는 내부적으로 핸드셰이크를
 * 진행하며, 이때 발생하는 서버/클라이언트 응답 패킷은 `writeBio`에 쌓이게
 * 됩니다.
 *    - 따라서 `decrypt()` 호출 후에는 반드시 반환된 데이터를 확인하여
 * 네트워크로 전송해야 합니다.
 *
 * 2. 버퍼 관리:
 *    - `BufferSize` (4096)는 1회 `SSL_read` 당 최대 평문 추출 크기입니다.
 *    - 대용량 데이터 수신 시 `decrypt()` 루프 내에서 평문이 모두 소진될 때까지
 * 호출됩니다.
 *
 * 3. 에러 처리:
 *    - 인증서 오류나 프로토콜 불일치 시 `SSL_do_handshake` 등에서 에러가
 * 발생하며, 이때 생성된 TLS Alert 패킷이 상대방에게 전달되어야 세션이
 * 정상적으로 종료됩니다.
 */
