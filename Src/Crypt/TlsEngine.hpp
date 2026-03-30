#pragma once
#include <cstdint>
#include <memory>
#include <openssl/ssl.h>
#include <string>
#include <vector>

/**
 * @file TlsEngine.hpp
 * @brief TLS 세션 및 컨텍스트 관리 엔진
 *
 * 이 파일은 TCP 통신 레이어 상단에서 전송 계층 보안을 제공합니다.
 * OpenSSL의 Memory BIO 패턴을 사용하여 소켓 I/O와 분리된 메모리 기반 암호화
 * 처리를 수행하며, 이를 통해 다양한 네트워크 프레임워크와의 유연한 통합을
 * 지원합니다.
 */

namespace TLS {

/**
 * @brief 서버용 컨텍스트 생성
 * @param certfile PEM 형식의 서버 인증서 파일 경로
 * @param keyfile PEM 형식의 서버 개인키 파일 경로
 * @param cafile (선택) 상호 인증(mTLS)을 위한 CA 파일 경로
 * @return SSL_CTX* 설정이 완료된 TLS 서버 컨텍스트 포인터
 */
SSL_CTX *ServerContext(const char *certfile = nullptr,
                       const char *keyfile = nullptr,
                       const char *cafile = nullptr);

/**
 * @brief 클라이언트용 컨텍스트 생성
 * @param certfile (선택) 클라이언트 인증서 경로
 * @param keyfile (선택) 클라이언트 개인키 경로
 * @param cafile (선택) 서버 검증을 위한 CA 파일 경로
 * @return SSL_CTX* 설정이 완료된 TLS 클라이언트 컨텍스트 포인터
 */
SSL_CTX *ClientContext(const char *certfile = nullptr,
                       const char *keyfile = nullptr,
                       const char *cafile = nullptr);

/**
 * @class Session
 * @brief TLS 세션 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. TLS::ServerContext() 또는 TLS::ClientContext()로 생성된 SSL_CTX를 생성자에
 * 전달하여 인스턴스를 생성합니다.
 * 2. Handshake()를 호출하여 초기 핸드셰이크 데이터를 생성하고, 상대방의 응답은
 * decrypt()를 통해 입력합니다.
 * 3. isHandshakeDone()이 true가 되면 encrypt()/decrypt()를 사용하여 보안 채널을
 * 통해 데이터를 송수신합니다.
 */
class Session {
public:
  /**
   * @param ctx SSL 컨텍스트
   * @param isServer 서버 모드 여부
   */
  Session(SSL_CTX *ctx, bool isServer = true);
  ~Session();

  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;
  Session(Session &&other) noexcept;
  Session &operator=(Session &&other) noexcept;

  /**
   * @return bool SSL 객체 유효성 여부
   */
  bool isValid() const { return ssl != nullptr; }

  /**
   * @return bool TLS 핸드셰이크 완료 여부
   */
  bool isHandshakeDone() const;

  /**
   * @brief 핸드셰이크 절차 진행 (초기 패킷 생성 등)
   * @return std::vector<uint8_t> 네트워크로 전송해야 할 핸드셰이크 데이터
   * @note 수신된 핸드셰이크 데이터는 decrypt()를 통해 내부로 입력해야 합니다.
   */
  std::vector<uint8_t> Handshake();

  /**
   * @param buffer 수신된 원시 데이터 (암호문)
   * @param len 데이터 길이
   * @return std::vector<uint8_t> 복호화된 평문 데이터 또는 핸드셰이크 응답
   * @note 핸드셰이크 중에는 평문 대신 내부 상태 전이 및 응답 패킷 생성이
   * 일어납니다.
   */
  std::vector<uint8_t> decrypt(const char *buffer, int len);

  /**
   * @param buffer 전송할 평문 데이터
   * @param len 데이터 길이
   * @return std::vector<uint8_t> TLS 레코드로 암호화된 데이터
   * @note 연결이 완전히 수립(isHandshakeDone)된 후에만 유의미한 암호문을
   * 반환합니다.
   */
  std::vector<uint8_t> encrypt(const char *buffer, int len);

  /**
   * @return std::vector<uint8_t> 보류 중인 암호화/핸드셰이크 데이터
   * @note 내부 writeBio를 비우고 네트워크로 전송할 데이터를 추출합니다.
   */
  std::vector<uint8_t> getHandshakeData();

private:
  void cleanup();
  std::vector<uint8_t> flushWriteBio();

  SSL *ssl = nullptr;
  BIO *readBio = nullptr;
  BIO *writeBio = nullptr;
  static constexpr int BufferSize = 4096;
};

} // namespace TLS

/**
 * @section Workflow Guide
 *
 * **[TlsEngine 통합 워크플로우 가이드]**
 *
 * 1. 세션 수립 단계:
 *    - 클라이언트: Session 생성 후 session.Handshake() 호출 -> 반환된 Client
 * Hello를 전송.
 *    - 서버: Client Hello 수신 시 session.decrypt()에 입력 -> 반환된 Server
 * Hello 등을 전송.
 *    - 이후 수신되는 데이터를 상호 간에 session.decrypt()에 입력하며 Handshake
 * 완료 대기.
 *
 * 2. 데이터 보호 단계:
 *    - session.isHandshakeDone() 확인.
 *    - 송신: session.encrypt() -> 결과물을 TCP 송신 버퍼에 저장 또는 즉시 전송.
 *    - 수신: TCP 수신 데이터 -> session.decrypt() -> 결과물(평문) 처리.
 *
 * 3. 주의사항:
 *    - Memory BIO 패턴은 소켓 에러를 직접 반환하지 않으므로,
 *      OpenSSL 함수 호출 후 session.getHandshakeData()를 통해
 *      상대방에게 알려야 할 TLS Alert 등이 있는지 확인하는 것이 중요합니다.
 */
