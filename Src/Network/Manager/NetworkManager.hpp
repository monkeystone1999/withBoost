#pragma once
#include "../NetworkProtocol.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <openssl/ssl.h>
#include <sigslot/signal.hpp>
/**
 * @class NetworkManager
 * @brief 네트워크 자구 처리 및 신호 분배 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 싱글톤 또는 전역 인스턴스로 생성되어 시스템 전반의 I/O 컨텍스트를
 * 제공합니다.
 * 2. 저수준 세션(`TextConnect`, `MediaConnect`)이 수신한 데이터를 Packets()
 * 메서드로 넘겨받습니다.
 * 3. 등록된 시그널 슬롯(sigslot)을 통해 UI 모델이나 로직 컨트롤러로 데이터를
 * 전파합니다.
 */
class NetworkManager {
public:
  /** @brief 카메라 설정, 메타데이터, AI 처리 결과를 전달하는 통합 시그널 */
  sigslot::signal<std::string, CameraPayload> CameraBridge_;
  /** @brief 사용자 인증 및 권한 확인 결과를 전달하는 시그널 */
  sigslot::signal<std::string, AuthBridgePayload> AuthBridge_;
  /** @brief 시스템 상태 및 장치 연결 정보를 전달하는 시그널 */
  sigslot::signal<Payload> StatusBridge_;
  /** @brief 이미지 수신 결과 확인 메시지를 발신하기 위한 브릿지 */
  sigslot::signal<const std::string &> OutboundImageBridge_;
  /** @brief UDP를 통해 수신된 미디어 조각 데이터를 전달하는 브릿지 */
  sigslot::signal<const std::string &, const ImageHeader &, const uint8_t *,
                  size_t>
      UdpImageBridge_;

  NetworkManager();
  ~NetworkManager();

  /**
   * @brief 수신된 패킷을 타입에 따라 분석 및 분배
   * @param Type 메시지 유형 (CAMERA, META, AI, IMAGE 등)
   * @param Body JSON 형식의 원시 페이로드 데이터
   * @note 내부적으로 nlohmann::json을 사용하여 파싱하며, 실패 시 해당 패킷을
   * 폐기합니다.
   */
  void Packets(MessageType Type, Payload Body);
  /** @return boost::asio::io_context& 비동기 작업을 위한 I/O 서비스 참조 */
  boost::asio::io_context &getIO() { return io_; }
  /** @brief TextConnect 엔진 인스턴스 등록 */
  void setTextConnect(class TextConnect *connect) { textConnect_ = connect; }
  /** @brief JIT 연결 시작 (AuthBridge에서 호출) */
  void startTextConnect();
  /** @brief 메시지 발신 (AuthBridge에서 호출) */
  void sendTextMessage(MessageType type, const std::string &jsonBody);
  /** @return SSL_CTX* TCP 보안 통신용 컨텍스트 포인터 */
  SSL_CTX *getTlsCtx() const { return tlsCtx_; }
  /** @return SSL_CTX* UDP 보안 제어용 컨텍스트 포인터 */
  SSL_CTX *getDtlsCtx() const { return dtlsCtx_; }
  /** @return SSL_CTX* 실시간 미디어 스트리밍 최적화 컨텍스트 포인터 */
  SSL_CTX *getMediaDtlsCtx() const { return mediaDtlsCtx_; }

private:
  boost::asio::io_context io_;
  SSL_CTX *tlsCtx_ = nullptr;
  SSL_CTX *dtlsCtx_ = nullptr;
  SSL_CTX *mediaDtlsCtx_ = nullptr;
  class TextConnect *textConnect_ = nullptr;
};

/**
 * @section Workflow Guide
 *
 * **[NetworkManager 연동 워크플로우]**
 *
 * 1. 초기화 단계:
 *    - `NetworkManager` 생성 시 OpenSSL 컨텍스트들이 클라이언트 모드로 자동
 * 구성됩니다.
 *    - 각 세션(`TextConnect` 등)은 `getIO()`와 `getTlsCtx()` 등을 호출하여
 * 통신을 준비합니다.
 *
 * 2. 수신 및 분배 단계:
 *    - 저수준 엔진이 `Packets(type, payload)`를 호출합니다.
 *    - `Packets` 내부에서 JSON 파싱 후, `ip` 등의 키를 기반으로 `CameraBridge_`
 * 등 적절한 시그널을 발생시킵니다.
 *    - UI 컨트롤러나 서비스 레이어는 미리 이 시그널에 `connect()` 되어 있어야
 * 합니다.
 *
 * 3. 자원 해제:
 *    - 파괴 시 할당된 모든 `SSL_CTX`를 안전하게 해제합니다.
 */
