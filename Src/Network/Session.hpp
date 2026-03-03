#pragma once
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <map>
#include <deque>
#include <vector>
#include <array>
#include <memory>
#include <thread>
#include "Header.hpp"
// 비동기적으로 동작을 할려면 해당 객체에 대한 생명주기가 보장이 되어야한다.

template<typename... Args>
nlohmann::json to_json(std::pair<std::string, Args>&&... pairs) { //  to_json : pair 묶음을 받아 nlohmann::json 으로 변환
    nlohmann::json j;
    (j.emplace(pairs.first, pairs.second), ...); // fold expression: 각 pair 를 json 에 삽입
    return j;
}

// ──────────────────────────────────────────────
//  MakePacket : json body → [헤더 5B | body] 직렬화
// ──────────────────────────────────────────────
inline std::vector<uint8_t> MakePacket(MessageType type, const nlohmann::json& body) {
    std::string json_str = body.dump();                              // json 객체를 string 으로 직렬화
    std::vector<uint8_t> packet(sizeof(PacketHeader) + json_str.size()); // 헤더 + 바디 크기 할당

    PacketHeader hdr;
    hdr.type        = type;
    hdr.body_length = static_cast<uint32_t>(json_str.size());       // 바디 길이를 헤더에 기록
    std::memcpy(packet.data(), &hdr, sizeof(PacketHeader));          // 패킷 앞 5바이트에 헤더 복사
    std::memcpy(packet.data() + sizeof(PacketHeader),                // 헤더 뒤에 json 바디 복사
                json_str.data(), json_str.size());
    return packet;
}


class ConnectServer {
public:
    ConnectServer(boost::asio::io_context& io,
                  const std::string ip,
                  const std::string port)
        : io_(io)
        , socket_(io)                                                // socket 을 io_context 에 등록
        , strand_(boost::asio::make_strand(io))                     // 이 Session 전용 strand 생성
    {
        boost::asio::ip::tcp::resolver resolver(io);                // ip:port → endpoint 변환기
        auto endpoint = resolver.resolve(ip, port);                 // 도메인/ip 를 실제 주소로 변환
        Message_.reserve(4096);                                      // 수신 버퍼 메모리 미리 확보
        Connect(endpoint);
    }

private:
    void Connect(auto endpoint) {
        boost::asio::async_connect(                                  // 비동기 TCP 연결 시도
            socket_, endpoint,
            boost::asio::bind_executor(strand_,                     // 콜백을 strand_ 위에서 실행 보장
                                       [this](boost::system::error_code ec, auto) {
                                           if (!ec) {
                                               ReadHeader();                               // 연결 성공 → 즉시 읽기 루프 시작
                                           } else {
                                               HandleError(ec);
                                           }
                                       }));
    }

    void ReadHeader() {
        boost::asio::async_read(                                     // 지정 크기가 꽉 찰 때까지 대기 후 콜백
            socket_,
            boost::asio::buffer(header_buf_.data(),                  // 수신 대상 버퍼: 고정 5바이트 array
                                sizeof(PacketHeader)),
            boost::asio::bind_executor(strand_,                     // 콜백을 strand_ 위에서 실행 보장
                                       [this](boost::system::error_code ec, std::size_t) {
                                           if (ec) { HandleError(ec); return; }

                                           BasePacketHeader =
                                               reinterpret_cast<PacketHeader*>(header_buf_.data()); // 5바이트를 PacketHeader 구조체로 해석
                                           ReadBody(BasePacketHeader->body_length);         // 헤더에서 읽은 body_length 로 바디 수신 시작
                                       }));
    }

    void ReadBody(uint32_t len) {
        if (len == 0) {                                              // 바디 없는 패킷 (ACK, SUCCESS 등)
            HeaderSwitch();
            ReadHeader();                                            // 바로 다음 헤더 대기
            return;
        }

        Message_.resize(len);                                        // 헤더가 알려준 크기만큼 버퍼 조정

        boost::asio::async_read(                                     // len 바이트가 전부 도착할 때까지 대기
            socket_,
            boost::asio::buffer(Message_.data(), len),              // 수신 대상 버퍼: Message_ vector
            boost::asio::bind_executor(strand_,                     // 콜백을 strand_ 위에서 실행 보장
                                       [this](boost::system::error_code ec, std::size_t) {
                                           if (ec) { HandleError(ec); return; }

                                           HeaderSwitch();                                  // 수신 완료 → 타입별 처리
                                           ReadHeader();                                    // 처리 완료 → 다음 헤더 대기 (루프)
                                       }));
    }

    void HeaderSwitch() {
        switch (BasePacketHeader->type) {
        case MessageType::ACK:
            break;
        case MessageType::SUCCESS:
            break;
        case MessageType::AI: {
            std::string json_str(Message_.begin(), Message_.end()); // 수신 바이트를 string 으로 변환
            auto j = nlohmann::json::parse(json_str);               // string → nlohmann::json 역직렬화
            // TODO: j["key"] 로 필드 접근
            break;
        }
        case MessageType::CAMERA: {
            std::string json_str(Message_.begin(), Message_.end()); // 수신 바이트를 string 으로 변환
            auto j = nlohmann::json::parse(json_str);               // string → nlohmann::json 역직렬화
            // TODO: 카메라 리스트 처리
            break;
        }
        case MessageType::FAIL:
            break;
        default:
            break;
        }
    }

    template<typename... Arg>
    void Write(MessageType type, std::pair<std::string, Arg>&&... pairs) {
        nlohmann::json body = to_json(                               // pair 묶음을 json 으로 변환
            std::forward<std::pair<std::string, Arg>>(pairs)...);

        auto packet = MakePacket(type, body);                       // json → [헤더|바디] vector<uint8_t>

        boost::asio::post(strand_,                                  // 람다를 strand_ 에 올려서 스레드 안전하게 실행
                          [this, pkt = std::move(packet)]() mutable {
                              bool idle = write_queue_.empty();                   // 현재 진행 중인 write 가 없으면 true
                              write_queue_.push_back(std::move(pkt));             // 큐에 패킷 추가 (move 로 복사 없이)
                              if (idle) DoWrite();                                // 유휴 상태일 때만 DoWrite 시작 (중복 방지)
                          });
    }

    void DoWrite() {
        boost::asio::async_write(                                    // buffer 전체가 전송될 때까지 대기 후 콜백
            socket_,
            boost::asio::buffer(write_queue_.front()),              // 큐 맨 앞 패킷을 buffer 로 참조
            boost::asio::bind_executor(strand_,                     // 콜백을 strand_ 위에서 실행 보장
                                       [this](boost::system::error_code ec, std::size_t) {
                                           if (ec) { HandleError(ec); return; }
                                           write_queue_.pop_front();                        // 전송 완료된 패킷 큐에서 제거
                                           if (!write_queue_.empty()) DoWrite();            // 남은 패킷 있으면 연속 전송 (큐 비면 종료)
                                       }));
    }
    void HandleError(boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted) return;    // 의도적 취소 (Close 호출 등) → 무시
        boost::system::error_code ignored;
        socket_.close(ignored);                                      // 소켓 닫기
    }

public:
    void Close() {
        boost::asio::post(strand_, [this]() {                       // strand_ 위에서 실행 → 읽기/쓰기와 충돌 없음
            boost::system::error_code ec;
            socket_.shutdown(                                        // 송수신 모두 종료 신호 전송
                boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_.close(ec);                                      // 소켓 닫기
        });
    }

private:
    boost::asio::io_context&    io_;                                // 공유 (참조) — 소유하지 않음
    boost::asio::ip::tcp::socket socket_;                           // Session 마다 고유 — 각자의 TCP 연결
    boost::asio::strand<boost::asio::io_context::executor_type> strand_; // Session 마다 고유 — 콜백 직렬화 보장

    std::array<uint8_t, sizeof(PacketHeader)> header_buf_{};        // 헤더 전용 고정 버퍼 (5바이트, 스택)
    std::vector<uint8_t>    Message_;                               // 바디 수신 버퍼 (가변, 힙)
    std::deque<std::vector<uint8_t>> write_queue_;                  // 송신 큐 — front pop 이 O(1)

    PacketHeader* BasePacketHeader = nullptr;                       // header_buf_ 를 재해석한 포인터
};


class SessionManager {
public:
    SessionManager() {
        work_guard_ = std::make_unique<WorkGuard>(
            boost::asio::make_work_guard(io_context_));             // io_context 에 가상 작업 등록 — run() 이 빈 상태에서 종료 안 되게 유지
        io_thread_ = std::thread([this] { io_context_.run(); });    // 이벤트 루프 시작 — 모든 async 콜백이 이 스레드에서 실행됨
    }

    ~SessionManager() {
        work_guard_.reset();                                         // 가상 작업 해제 → run() 종료 허용
        if (io_thread_.joinable())
            io_thread_.join();                                       // io_thread_ 완전 종료까지 대기
    }

    void AddSession(const std::string& name,
                    const std::string& ip,
                    const std::string& port) {
        manager_[name] = std::make_unique<ConnectServer>(           // io_context_ 를 참조로 주입
            io_context_, ip, port);
    }

    ConnectServer* GetSession(const std::string& name) {
        auto it = manager_.find(name);
        return it != manager_.end() ? it->second.get() : nullptr;
    }

private:
    using WorkGuard = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;

    boost::asio::io_context    io_context_;                         // 이벤트 루프 — SessionManager 가 소유
    std::unique_ptr<WorkGuard> work_guard_;                         // run() 이 빈 상태에서 종료 안 되게 유지
    std::thread                io_thread_;                          // io_context_.run() 전용 백그라운드 스레드

    std::map<std::string, std::unique_ptr<ConnectServer>> manager_; // 이름 → Session 매핑
};
