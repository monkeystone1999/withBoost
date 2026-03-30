#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <openssl/ssl.h>
#include <string>
#include <vector>

/**
 * @file DtlsEngine.hpp
 * @brief DTLS 및 MediaDTLS 공용 엔진
 *
 * 이 파일은 UDP 통신 레이어와 직접 결합되어 데이터의 기밀성과 무결성을
 * 보장합니다. 핵심 디자인 패턴으로 OpenSSL의 Memory BIO를 사용하여 네트워크
 * I/O와 가상화된 메모리 버퍼를 분리, 비동기 이벤트 루프 또는 외부 소켓
 * 라이브러리와의 통합이 용이하도록 설계되었습니다.
 */

namespace DTLS {

/**
 * @brief 쿠키 초기화 (앱 시작 시 1회 호출 필수)
 * @note 서버 모드에서 클라이언트의 주소 변조를 방지하기 위한 Stateless Cookie
 * 생성용 시드(Seed)를 난수로 초기화합니다.
 */
void InitCookie();

/**
 * @brief 서버용 컨텍스트 생성
 * @param certfile PEM 형식의 서버 인증서 파일 경로
 * @param keyfile PEM 형식의 서버 개인키 파일 경로
 * @param cafile (선택) 클라이언트 인증서 검증을 위한 CA 파일 경로
 * @return SSL_CTX* 설정이 완료된 서버 컨텍스트 포인터
 */
SSL_CTX *ServerContext(const char *certfile = nullptr,
                       const char *keyfile = nullptr,
                       const char *cafile = nullptr);

/**
 * @brief 클라이언트용 컨텍스트 생성
 * @param certfile (선택) 클라이언트 인증서 파일 경로
 * @param keyfile (선택) 클라이언트 개인키 파일 경로
 * @param cafile (선택) 서버 인증서 검증을 위한 CA 파일 경로
 * @return SSL_CTX* 설정이 완료된 클라이언트 컨텍스트 포인터
 */
SSL_CTX *ClientContext(const char *certfile = nullptr,
                       const char *keyfile = nullptr,
                       const char *cafile = nullptr);

/**
 * @class Session
 * @brief 표준 DTLS 세션 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. DTLS::ServerContext() 또는 DTLS::ClientContext()를 통해 생성된 SSL_CTX를
 * 사용하여 인스턴스를 생성합니다.
 * 2. 네트워크로부터 수신된 패킷이 있다면 decrypt() 또는 Handshake()의 인자로
 * 전달하고, 반환되는 데이터를 네트워크로 전송합니다.
 * 3. isHandshakeDone()이 true가 되면 encrypt()/decrypt()를 통해 안전한 데이터
 * 통신을 수행합니다.
 */
class Session {
public:
  /**
   * @param ctx SSL 컨텍스트
   * @param isServer 서버 모드 여부
   * @param peerIdent (선택) 상대방 식별 정보
   */
  Session(SSL_CTX *ctx, bool isServer = true,
          std::vector<uint8_t> peerIdent = {});
  ~Session();

  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;
  Session(Session &&other) noexcept;
  Session &operator=(Session &&other) noexcept;

  /**
   * @return bool SSL 객체 생성 성공 여부
   */
  bool isValid() const { return ssl != nullptr; }

  /**
   * @return bool 핸드셰이크 절차 완료 여부
   */
  bool isHandshakeDone() const;

  /**
   * @param peerIdent 상대방 식별을 위한 원시 데이터 (IP/Port 등)
   * @note 내부적으로 쿠키 검증 또는 세션 식별에 사용될 수 있습니다.
   */
  void setPeer(std::vector<uint8_t> peerIdent);

  /**
   * @brief 핸드셰이크 진행 (수신 데이터 없는 경우)
   * @return std::vector<uint8_t> 네트워크로 즉시 전송해야 할 핸드셰이크 패킷
   * @note 타이머 만료에 따른 재전송 패킷 등이 생성될 수 있으므로 정기적으로
   * 확인이 필요합니다.
   */
  std::vector<uint8_t> Handshake();

  /**
   * @param buffer 수신된 원시 패킷 데이터
   * @param size 데이터 크기
   * @return std::vector<uint8_t> 복호화된 평문 데이터 또는 핸드셰이크 응답 패킷
   * @note 반환된 데이터가 암호화된 핸드셰이크 패킷인지 여부는 내부 상태와
   * getHandshakeData() 호출로 판별합니다.
   */
  std::vector<uint8_t> decrypt(const char *buffer, size_t size);

  /**
   * @param buffer 전송할 평문 데이터
   * @param size 데이터 크기
   * @return std::vector<uint8_t> DTLS 레코드로 캡슐화된 암호화 패킷
   * @note 핸드셰이크가 완료되지 않은 상태에서 호출 시 빈 결과를 반환합니다.
   */
  std::vector<uint8_t> encrypt(const char *buffer, size_t size);

  /**
   * @return std::vector<uint8_t> 보류 중인 핸드셰이크 데이터
   * @note 내부 writeBio에 쌓인 데이터를 추출하여 네트워크 레이어로 전달하기
   * 위해 사용합니다.
   */
  std::vector<uint8_t> getHandshakeData();

private:
  void cleanup();
  std::vector<uint8_t> flushWriteBio();

  SSL *ssl = nullptr;
  BIO *readBio = nullptr;
  BIO *writeBio = nullptr;
  std::vector<uint8_t> peer;
  static constexpr int BufferSize = 4096;
};

} // namespace DTLS

namespace MediaDTLS {

/**
 * @brief 서버용 컨텍스트 생성 (미디어 전용)
 * @param certfile 서버 인증서 경로
 * @param keyfile 서버 개인키 경로
 * @param cafile 클라이언트 검증용 CA 경로
 * @return SSL_CTX* 미디어 데이터 최적화 설정이 적용된 컨텍스트
 */
SSL_CTX *ServerContext(const char *certfile = nullptr,
                       const char *keyfile = nullptr,
                       const char *cafile = nullptr);

/**
 * @brief 클라이언트용 컨텍스트 생성 (미디어 전용)
 * @param certfile 클라이언트 인증서 경로
 * @param keyfile 클라이언트 개인키 경로
 * @param cafile 서버 검증용 CA 경로
 * @return SSL_CTX* 미디어 데이터 최적화 설정이 적용된 컨텍스트
 */
SSL_CTX *ClientContext(const char *certfile = nullptr,
                       const char *keyfile = nullptr,
                       const char *cafile = nullptr);

/**
 * @class Session
 * @brief 미디어 스트리밍 최적화 DTLS 세션 클래스
 *
 * **Standard Usage Methodology:**
 * 1. MediaDTLS 컨텍스트를 사용하여 세션을 초기화합니다.
 * 2. Handshake(incoming) 함수를 통해 초기 보안 연결을 수립합니다.
 * 3. encrypt()/decrypt()를 사용하여 실시간 미디어 청크를 보호 전송합니다.
 */
class Session {
public:
  /**
   * @param ctx SSL 컨텍스트
   * @param isServer 서버 모드 여부
   * @note 내부적으로 1200바이트의 MTU 힌트를 설정하여 UDP 단편화를 방지합니다.
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
   * @return bool 핸드셰이크 완료 여부
   */
  bool isHandshakeDone() const;

  /**
   * @brief 핸드셰이크 절차 진행
   * @param incoming 상대방으로부터 수신된 DTLS 패킷
   * @return std::vector<uint8_t> 네트워크로 전송해야 할 응답 패킷
   * @note 핸드셰이크 단계에서는 수신 데이터가 없더라도 수시로 호출하여 재전송을
   * 처리해야 합니다.
   */
  std::vector<uint8_t> Handshake(const std::vector<uint8_t> &incoming = {});

  /**
   * @return std::string 합의된 암호 알고리즘 명칭 (예: "AES128-GCM-SHA256")
   */
  std::string getNegotiatedCipher() const;

  /**
   * @param buffer 암호화할 미디어 평문 데이터
   * @param len 데이터 길이
   * @return std::vector<uint8_t> DTLS 암호화된 UDP 페이로드
   * @note MTU 크기에 맞게 미리 분할된 데이터를 입력하는 것을 권장합니다.
   */
  std::vector<uint8_t> encrypt(const char *buffer, int len);

  /**
   * @param buffer 수신된 DTLS 암호문 패킷
   * @param len 패킷 길이
   * @return std::vector<uint8_t> 복호화된 미디어 평문
   * @note 핸드셰이크 패킷이 입력된 경우 평문 결과는 비어 있으며, 내부 상태만
   * 전이됩니다.
   */
  std::vector<uint8_t> decrypt(const char *buffer, int len);

private:
  void cleanup();
  std::vector<uint8_t> flushWriteBio();

  SSL *ssl = nullptr;
  BIO *readBio = nullptr;
  BIO *writeBio = nullptr;

  static constexpr int BufferSize = 4096;
};
} // namespace MediaDTLS

/**
 * @section Workflow Guide
 *
 * **[통합 워크플로우 예시: UDP 송수신 루프]**
 *
 * 1. 초기화:
 *    - DTLS::InitCookie() 호출
 *    - SSL_CTX 생성 (ServerContext/ClientContext)
 *    - DTLS::Session 또는 MediaDTLS::Session 인스턴스 생성
 *
 * 2. 핸드셰이크 루프:
 *    - 클라이언트: 세션 생성 후 즉시 session.Handshake() 호출 -> 반환된
 * 데이터를 sendto()로 전송
 *    - 서버/클라이언트 공통: recvfrom()으로 받은 데이터를 session.decrypt()
 * 또는 session.Handshake(data)에 입력
 *    - session.isHandshakeDone()이 true가 될 때까지 반복
 *
 * 3. 데이터 송수신:
 *    - 송신: session.encrypt(plain, len) -> 반환된 encrypted 데이터를
 * sendto()로 전송
 *    - 수신: recvfrom() -> session.decrypt(encrypted, len) -> 반환된 plain
 * 데이터 처리
 *
 * **주의사항:**
 * - 비차단(Non-blocking) 소켓 사용 시, session.Handshake()를 주기적으로
 * 호출하여 패킷 손실에 따른 DTLS 재전송 메커니즘이 작동하도록 해야 합니다.
 */
