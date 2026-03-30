#pragma once
#include "../Crypt/TlsEngine.hpp"
#include "../Thread/ThreadEngine.hpp"
#include "NetworkProtocol.hpp"
#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <variant>

/**
 * @file NetworkEngine.hpp
 * @brief 저수준 네트워크 통신 엔진 (Session, Connect 계열)
 *
 * 이 파일은 Boost.Asio를 기반으로 한 비동기 TCP/UDP 소켓 통신 레이어를
 * 정의합니다. 서비스 레이어와 도메인 레이어 사이에서 원시 바이트 스트림과
 * 패킷의 송수신을 담당하며, 아키텍처 상 'NetworkManager'에 의해 제어되는 저수준
 * 통신 엔진 역할을 수행합니다.
 */

class NetworkManager;

// ─────────────────────────────────────────────────────────────
// Session — TCP/UDP 저수준 통신 세션
// ─────────────────────────────────────────────────────────────

/**
 * @class Session
 * @brief TCP/UDP 프로토콜별 비동기 소켓 세션 템플릿
 */
template <Protocol P> class Session;

/**
 * @class Session<Protocol::TCP>
 * @brief TCP 전용 비동기 통신 세션
 *
 * **Standard Usage Methodology:**
 * 1. boost::asio::io_context와 strand를 사용하여 인스턴스를 생성합니다.
 * 2. Connect()를 호출하여 서버와 물리적 연결을 수립합니다.
 * 3. Read()와 Write()를 비동기적으로 호출하여 데이터를 송수신하며, stop()으로
 * 세션을 종료합니다.
 */
template <>
class Session<Protocol::TCP>
    : public std::enable_shared_from_this<Session<Protocol::TCP>> {
public:
  /**
   * @param io Boost.Asio I/O 컨텍스트
   */
  explicit Session(boost::asio::io_context &io)
      : io_(io), strand_(boost::asio::make_strand(io)), socket_(io) {};

  /**
   * @param ip 접속할 서버 IP 주소
   * @param port 접속할 서버 포트 번호
   * @note 내부적으로 전용 strand에서 비동기 접속을 시도합니다.
   */
  void Connect(const std::string ip, const uint16_t port) {
    boost::asio::ip::tcp::endpoint endpoint_(boost::asio::ip::make_address(ip),
                                             port);
    socket_.async_connect(
        endpoint_, boost::asio::bind_executor(
                       strand_, [](const boost::system::error_code &ec) {}));
  }

  /**
   * @param data 수신 데이터를 담을 버퍼 (shared_ptr)
   * @param After 수신 완료 후 실행할 콜백 함수
   * @note 지정된 버퍼 크기만큼 데이터가 채워질 때까지 리드를 대기합니다.
   */
  void Read(std::shared_ptr<std::vector<uint8_t>> data,
            std::function<void()> After) {
    boost::asio::async_read(
        socket_, boost::asio::buffer(data->data(), data->size()),
        boost::asio::bind_executor(
            strand_,
            [self = shared_from_this(), data,
             After](const boost::system::error_code &ec, std::size_t bytes) {
              if (!ec)
                After();
            }));
  }

  /**
   * @param data 수신 데이터를 담을 버퍼
   * @note 콜백 없이 연속적인 수신 대기를 할 때 사용합니다.
   */
  void Read(std::shared_ptr<std::vector<uint8_t>> data) {
    boost::asio::async_read(socket_,
                            boost::asio::buffer(data->data(), data->size()),
                            boost::asio::bind_executor(
                                strand_, [](const boost::system::error_code &ec,
                                            std::size_t bytes) {}));
  }

  /**
   * @param data 전송할 원시 데이터 벡터
   * @note 전송 중 버퍼 유효성을 보장하기 위해 내부적으로 copy-on-write 방식을
   * 사용합니다.
   */
  void Write(std::vector<uint8_t> data) {
    auto buf = std::make_shared<std::vector<uint8_t>>(std::move(data));
    boost::asio::async_write(
        socket_, boost::asio::buffer(*buf),
        boost::asio::bind_executor(
            strand_,
            [self = shared_from_this(),
             buf](const boost::system::error_code &ec, std::size_t bytes) {}));
  }

  /**
   * @note 소켓을 즉시 닫고 모든 대기 중인 I/O 작업을 취소합니다.
   */
  void stop() {
    boost::system::error_code ec;
    socket_.close(ec);
  }

private:
  boost::asio::io_context &io_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
};

/**
 * @class Session<Protocol::UDP>
 * @brief UDP 전용 비동기 통신 세션
 *
 * **Standard Usage Methodology:**
 * 1. 생성 직후 Bind()를 호출하여 로컬 통신 포트를 할당받습니다.
 * 2. StartReceive()를 호출하여 비동기 수신 루프를 활성화합니다.
 * 3. Write()를 사용하여 상대방 엔드포인트로 패킷을 전송합니다.
 */
template <>
class Session<Protocol::UDP>
    : public std::enable_shared_from_this<Session<Protocol::UDP>> {
public:
  /**
   * @param io Boost.Asio I/O 컨텍스트
   */
  explicit Session(boost::asio::io_context &io)
      : io_(io), strand_(boost::asio::make_strand(io)),
        socket_(io, boost::asio::ip::udp::v4()) {}

  /**
   * @param port 바인딩할 로컬 포트 (0이면 OS 임의 할당)
   */
  void Bind(uint16_t port) {
    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(), port);
    socket_.bind(endpoint);
  }

  /** @brief 임의 포트로 바인딩 */
  void Bind() {
    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(), 0);
    socket_.bind(endpoint);
  }

  /**
   * @return uint16_t 할당된 실제 로컬 포트 번호
   */
  uint16_t LocalPort() const { return socket_.local_endpoint().port(); }

  /**
   * @param ip 목적지 IP
   * @param port 목적지 포트
   */
  void Connect(const std::string &ip, uint16_t port) {
    endpoint_ =
        boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(ip), port);
  }

  /**
   * @param data 전송할 데이터
   * @note UDP는 비연결형이므로 Connect()로 설정된 목적지로 즉시 전송됩니다.
   */
  void Write(std::vector<uint8_t> data) {
    auto buf = std::make_shared<std::vector<uint8_t>>(std::move(data));
    socket_.async_send_to(boost::asio::buffer(*buf), endpoint_,
                          boost::asio::bind_executor(
                              strand_, [self = shared_from_this(), buf](
                                           const boost::system::error_code &ec,
                                           std::size_t bytes) {}));
  }

  using ReceiveCallback =
      std::function<void(std::shared_ptr<std::vector<uint8_t>>, std::size_t,
                         boost::asio::ip::udp::endpoint)>;

  /**
   * @param cb 데이터 수신 시 호출될 콜백 함수
   * @note 한 번 호출하면 세션 종료 시까지 내부적으로 doReceive 루프가
   * 반복됩니다.
   */
  void StartReceive(ReceiveCallback cb) {
    receiveCb_ = std::move(cb);
    doReceive();
  }

  void stop() {
    boost::system::error_code ec;
    socket_.close(ec);
  }

private:
  void doReceive() {
    auto buf = std::make_shared<std::vector<uint8_t>>(2048);
    auto sender = std::make_shared<boost::asio::ip::udp::endpoint>();
    socket_.async_receive_from(
        boost::asio::buffer(*buf), *sender,
        boost::asio::bind_executor(
            strand_,
            [self = shared_from_this(), buf,
             sender](const boost::system::error_code &ec, std::size_t bytes) {
              if (!ec && bytes > 0 && self->receiveCb_) {
                self->receiveCb_(buf, bytes, *sender);
              }
              if (!ec)
                self->doReceive();
            }));
  }

  boost::asio::io_context &io_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint endpoint_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  ReceiveCallback receiveCb_;
};

// ─────────────────────────────────────────────────────────────
// Connection Handlers — 고수준 세션 엔진
// ─────────────────────────────────────────────────────────────

class MediaConnect;

/**
 * @class TextConnect
 * @brief TCP 기반 제어 메시지 및 텍스트 데이터 핸들러
 *
 * **Standard Usage Methodology:**
 * 1. NetworkManager 및 서버 주소 정보를 통해 생성합니다.
 * 2. run()을 호출하여 비동기 루프를 시작하고 Read()를 통해 수신 대기 상태로
 * 진입합니다.
 * 3. Send()를 사용하여 특정 타입과 바디를 가진 패킷을 전송합니다.
 */
class TextConnect : public std::enable_shared_from_this<TextConnect> {
public:
  TextConnect(NetworkManager &Manager, std::string ip, uint16_t port);
  void connect();

  /**
   * @param pool (선택) I/O 작업을 수행할 스레드 풀
   * @note pool이 nullptr인 경우 메인 이벤트 루프를 직접 점유합니다.
   */
  void run(ThreadEngine *pool = nullptr);

  void setMediaConnect(std::shared_ptr<MediaConnect> media);

  /** @brief 메시지 헤더부터 수신 시작 (재귀 호출) */
  void Read();

  /**
   * @param type 메시지 타입 (LOGIN, AI 등)
   * @param jsonBody JSON 형식의 페이로드 바디
   */
  void Send(MessageType type, const std::string &jsonBody);

  void SendImageReceiveResult(const std::string &jsonBody);
  const std::string &getCurrentImageIp() const { return currentImageIp_; }
  void stop();

private:
  void startHandshake();
  void onHandshakeRead();
  void sendLogin();

  uint16_t getMediaPort() const;
  boost::asio::io_context &io_;
  std::string ip_;
  uint16_t port_;
  std::shared_ptr<Session<Protocol::TCP>> TCPSession_;
  NetworkManager &NetworkManager_;
  ThreadEngine *pool_ = nullptr;
  std::shared_ptr<MediaConnect> MediaConnect_;
  std::string currentImageIp_;

  // TLS Layer
  std::unique_ptr<TLS::Session> tlsSession_;
  bool handshakeInitiated_ = false;

  // Pending Messages (waiting for handshake)
  struct PendingMessage {
    MessageType type;
    std::string jsonBody;
  };
  std::deque<PendingMessage> pendingMessages_;
};

/**
 * @class MediaConnect
 * @brief UDP 기반 미디어 데이터 스트리밍 핸들러
 *
 * **Standard Usage Methodology:**
 * 1. 인스턴스 생성 시 로컬 UDP 포트를 자동으로 바인딩합니다.
 * 2. run() 호출 후 Read()를 실행하여 미디어 패킷 수신 대기 루프를 활성화합니다.
 * 3. onReceive()를 통해 수신된 조각화된 이미지 데이터를 NetworkManager의
 * 브릿지로 전달합니다.
 */
class MediaConnect : public std::enable_shared_from_this<MediaConnect> {
public:
  explicit MediaConnect(NetworkManager &Manager);
  uint16_t LocalPort() const;
  void run(ThreadEngine *pool = nullptr);
  void Read();
  void Write(std::vector<uint8_t> data);
  void stop();

private:
  /**
   * @param buf 수신된 원시 데이터
   * @param bytes 수신 바이트 수
   * @param sender 수신처 엔드포인트 정보
   * @note 이미지 헤더 유효성을 검사한 뒤 UdpImageBridge를 통해 상위 레이어로
   * 전달합니다.
   */
  void onReceive(std::shared_ptr<std::vector<uint8_t>> buf, std::size_t bytes,
                 boost::asio::ip::udp::endpoint sender);

  boost::asio::io_context &io_;
  std::shared_ptr<Session<Protocol::UDP>> UDPSession_;
  NetworkManager &NetworkManager_;
  ThreadEngine *pool_ = nullptr;
};

/**
 * @section Workflow Guide
 *
 * **[NetworkEngine 연동 워크플로우]**
 *
 * 1. 통합 준비:
 *    - `NetworkManager` 인스턴스를 준비하고 IO context를 공유합니다.
 *    - `TextConnect`를 통해 서버와 영구적인 TCP 제어 채널을 연결합니다.
 *
 * 2. 수신 파이프라인:
 *    - `TextConnect::Read()` -> 메시지 파싱 -> `NetworkManager::Packets` 호출.
 *    - 미디어의 경우 `MediaConnect::Read()` -> UDP 수신 -> `UdpImageBridge`
 * 호출.
 *
 * 3. 송신 파이프라인:
 *    - 제어 명령: `TextConnect::Send(type, json)` 호출.
 *    - 미디어 명령: `MediaConnect::Write(data)` 호출.
 *
 * **주의사항:**
 * - 모든 Read/Write는 Boost.Asio의 비동기 핸들러 상에서 동작하므로,
 *   콜백 레벨에서의 블로킹 작업을 피해야 전체 통신 성능이 유지됩니다.
 */
