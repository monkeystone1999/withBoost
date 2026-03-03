# Boost.Asio 고성능 TCP 클라이언트

## 핵심 개념: Strand

모든 비동기 작업은 **strand** 위에서 실행되어야 수동 잠금 없이 데이터 경쟁을 방지할 수 있습니다.
소켓을 건드리는 모든 작업은 `strand_`를 통해 디스패치되며, 다른 스레드에서 직접 호출되지 않습니다.

```cpp
class Session {
    boost::asio::io_context&        io_;        // ← 공유 (모든 Session이 같은 것 사용)
    boost::asio::ip::tcp::socket    socket_;    // ← Session마다 고유
    boost::asio::strand<...>        strand_;    // ← Session마다 고유
    boost::asio::streambuf          read_buf_;  // ← Session마다 고유
    std::deque<std::string>         write_queue_; // ← Session마다 고유
};
```

---

## 다중 Session에서 각 값의 공유/고유 여부

클라이언트가 여러 서버에 동시 연결하거나 Session Pool을 운용할 때 핵심 질문은 **"이 값이 Session마다 따로 존재하는가?"** 입니다.

| 멤버 | Session마다 고유? | 이유 |
|---|---|---|
| `io_context&` | **공유** (참조) | 이벤트 루프 자체는 하나면 충분. 여러 Session이 같은 `io_context`에 작업을 등록함 |
| `tcp::socket` | **고유** | 각 Session은 별개의 TCP 연결이므로 소켓이 달라야 함 |
| `strand_` | **고유** | Strand는 소켓 하나의 직렬화 보장용. 다른 Session의 strand와 섞이면 안 됨 |
| `read_buf_` / `body_buf_` | **고유** | 서버마다 받는 데이터가 다르므로 버퍼도 분리 필요 |
| `write_queue_` | **고유** | 각 연결의 송신 큐는 독립적으로 관리되어야 함 |
| `header_` (PacketHeader) | **고유** | 수신 중인 헤더는 Session별로 다름 |

---

## 1. 연결 — `async_connect`

```cpp
void connect(const std::string& ip, const std::string& port) {
    boost::asio::ip::tcp::resolver resolver(io_);
    auto endpoints = resolver.resolve(ip, port);

    boost::asio::async_connect(socket_, endpoints,
        boost::asio::bind_executor(strand_,
            [this](boost::system::error_code ec, auto) {
                if (!ec) on_connect();
                else     handle_error(ec);
            }));
}
```

> **다중 Session 관점**: Session A는 서버 192.168.0.1:9000에, Session B는 192.168.0.2:9001에 각각 `async_connect`를 호출합니다. `socket_`과 `strand_`가 고유하므로 서로 간섭하지 않습니다.

---

## 2. 읽기 — `async_read`

### 고정 크기 헤더 → 이후 본문 읽기

```cpp
void read_header() {
    boost::asio::async_read(socket_,
        boost::asio::buffer(&header_, sizeof(PacketHeader)),
        boost::asio::bind_executor(strand_,
            [this](boost::system::error_code ec, std::size_t) {
                if (ec) { handle_error(ec); return; }
                read_body(header_.body_length);
            }));
}

void read_body(uint32_t len) {
    body_buf_.resize(len);
    boost::asio::async_read(socket_,
        boost::asio::buffer(body_buf_),
        boost::asio::bind_executor(strand_,
            [this](boost::system::error_code ec, std::size_t) {
                if (ec) { handle_error(ec); return; }
                on_message(body_buf_);
                read_header(); // 루프: 다음 읽기를 항상 예약
            }));
}
```

> **다중 Session 관점**: Session A가 100바이트짜리 메시지를 읽는 동안 Session B는 2000바이트짜리 메시지를 읽을 수 있습니다. `body_buf_`가 Session마다 고유하기 때문에 버퍼 충돌이 없습니다. 단 `io_context`는 공유되어 있으므로 스레드풀의 어떤 스레드든 콜백을 처리할 수 있고, `strand_`가 각 Session 내부의 순서를 보장합니다.

---

## 3. 쓰기 — 큐를 사용한 `async_write`

쓰기가 진행 중일 때 또 다른 `async_write`를 호출하면 안 됩니다 — 큐를 사용하세요.

```cpp
// 스레드 안전 진입점: 쓰기 작업을 strand 위에 post
void write(std::string message) {
    boost::asio::post(strand_,
        [this, msg = std::move(message)]() mutable {
            bool idle = write_queue_.empty();
            write_queue_.push_back(std::move(msg));
            if (idle) do_write();
        });
}

// strand 내부에서만 호출됨
void do_write() {
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_queue_.front()),
        boost::asio::bind_executor(strand_,
            [this](boost::system::error_code ec, std::size_t) {
                if (ec) { handle_error(ec); return; }
                write_queue_.pop_front();
                if (!write_queue_.empty()) do_write(); // 다음 쓰기 체인
            }));
}
```

> **다중 Session 관점**: `write_queue_`가 Session마다 고유하므로 Session A의 송신 큐가 가득 차도 Session B의 송신에는 전혀 영향이 없습니다. 외부 스레드에서 `write()`를 호출해도 `post(strand_, ...)`를 통해 해당 Session의 strand로만 전달됩니다.

---

## 4. 종료 — `boost::asio::post`

소켓을 다른 스레드에서 직접 닫지 말고 strand에 post하세요.

```cpp
void close() {
    boost::asio::post(strand_, [this]() {
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    });
}
```

> **다중 Session 관점**: Session A를 닫더라도 `strand_`가 고유하므로 Session B의 읽기/쓰기 작업에 영향을 주지 않습니다.

---

## 5. `io_context` 실행

컨텍스트는 스레드풀에서 실행하세요 — 호출 스레드에서 실행하지 마세요.

```cpp
// 최소 구성: 단일 백그라운드 스레드
boost::asio::io_context io;
auto work = boost::asio::make_work_guard(io); // 유휴 상태에서도 유지
std::thread io_thread([&io]{ io.run(); });

// 고성능: 스레드풀 (CPU 코어당 스레드 1개)
const int N = std::thread::hardware_concurrency();
std::vector<std::thread> pool;
for (int i = 0; i < N; ++i)
    pool.emplace_back([&io]{ io.run(); });
```

> **다중 Session 관점**: `io_context` 하나를 모든 Session이 공유합니다. 스레드풀로 실행하면 Session A의 콜백과 Session B의 콜백이 병렬로 처리될 수 있습니다. 각 Session의 `strand_`가 **같은 Session 내부**의 순서만 보장하면 되므로, Session 간 병렬성은 완전히 허용됩니다.

---

## 개발 패턴

### 패턴 A — 단일 Session (가장 단순)
서버 연결 하나당 `Session` 객체 하나.
적합한 경우: 서버 하나와 통신하는 단순한 클라이언트.

```
앱  →  Session  →  TCP 소켓
```

### 패턴 B — Session Pool
N개의 Session을 미리 생성하고, 요청마다 하나를 선택 (라운드로빈 또는 최소 부하).
적합한 경우: 처리량이 높은 클라이언트, 동시 송신이 많은 HTTP 형태의 요청/응답.

```
앱  →  SessionPool [Session0, Session1, ...]  →  TCP 소켓 ×N
```

각 Session이 독립적인 `socket_`, `strand_`, `write_queue_`, 버퍼를 가지지만 `io_context`는 하나를 공유합니다.

### 패턴 C — 재연결 루프
연결을 지수 백오프 방식의 재시도 루프로 감쌉니다.
적합한 경우: 서버 재시작 후에도 살아있어야 하는 장수 연결.

```cpp
void reconnect(int delay_ms = 500) {
    boost::asio::steady_timer timer(io_, std::chrono::milliseconds(delay_ms));
    timer.async_wait([this, delay_ms](auto ec) {
        if (!ec) connect(ip_, port_);
    });
}

void handle_error(boost::system::error_code ec) {
    if (ec != boost::asio::error::operation_aborted)
        reconnect(std::min(delay_ms_ * 2, 30000)); // 최대 30초
}
```

### 패턴 D — 코루틴 (C++20 / Asio 코루틴)
가장 깔끔한 비동기 코드 — 동기 코드처럼 읽히고, 콜백 중첩 없음.
Asio ≥ 1.21 및 `-std=c++20` 필요.

```cpp
boost::asio::awaitable<void> run_session(tcp::socket socket) {
    PacketHeader header;
    co_await boost::asio::async_read(socket,
        boost::asio::buffer(&header, sizeof(header)), boost::asio::use_awaitable);

    std::vector<uint8_t> body(header.body_length);
    co_await boost::asio::async_read(socket,
        boost::asio::buffer(body), boost::asio::use_awaitable);

    // body 처리...
}
```

---

## PacketHeader 통합

기존 `PacketHeader` (5바이트: 타입 1B + 길이 4B)는 이 패턴에 그대로 맞습니다.

```
[ MessageType (1B) ][ body_length (4B, 일반 uint32) ][ JSON 본문... ]

read_header()  →  body_length 디코딩  →  read_body(len)  →  on_message()  →  루프
```

`body_length`는 TCP 본문 안에 있는 **애플리케이션 데이터**이며 OS는 이 값을 건드리지 않습니다. 서버와 클라이언트 모두 직접 제어하므로, 양쪽에서 `body_length`를 네이티브 정수로 그냥 쓰고 읽으면 됩니다 — `ntohl` 변환이 필요하지 않습니다.