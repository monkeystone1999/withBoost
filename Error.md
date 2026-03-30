
# 이미지 수신 프로토콜 및 서버 동작 가이드

## 1. 이미지 수신 방식 (Client Logic)
클라이언트는 이미지를 효율적으로 처리하기 위해 **TCP(제어)**와 **UDP(데이터)** 채널을 동시에 사용합니다.

1.  **메타데이터 수신 (TCP)**: 서버는 이미지를 보내기 전, `META` (0x09) 또는 `IMAGE` (0x0A) 타입의 JSON을 TCP 채널로 먼저 보냅니다. 클라이언트는 이를 통해 어떤 기기의 이미지가 올지 미리 파악합니다.
2.  **UDP 채널 개방**: 클라이언트는 지정된 포트로 UDP 소켓을 열고, 서버와 **DTLS 핸드셰이크**를 수행합니다. (서버는 DTLS Server, 클라이언트는 DTLS Client 역할을 수행)
3.  **조각 수신 및 복호화**: 서버가 보낸 UDP 패킷에서 **평문 헤더(7B)**를 읽어 프레임 번호와 조각 순서를 확인하고, 뒤따르는 **암호화된 데이터**를 DTLS로 복호화합니다.
4.  **이미지 재조립**: 모든 조각(`SequenceNumber` 0 ~ `MaxSequenceNumber`)이 모이면 하나의 JPEG 파일로 합쳐 UI에 표시합니다.
5.  **타이머 관리**: 첫 조각 수신 후 50ms 이내에 모든 조각이 오지 않으면 유실된 것으로 판단합니다.

## 2. 서버가 해줘야 하는 동작 (Server Requirements)
서버는 클라이언트가 이미지를 정상적으로 복구할 수 있도록 아래 규칙을 반드시 준수해야 합니다.

1.  **패킷 구조**: 각 UDP 패킷은 반드시 `ImageHeader` (구조체 크기 7바이트)로 시작해야 합니다.
    - `type` (1 byte): `0x0A` (IMAGE)
    - `length` (4 bytes): 전체 패킷 길이
    - `FrameNumber` (1 byte): 프레임마다 증가하는 ID
    - `SequenceNumber` (1 byte): 현재 조각 번호 (0부터 시작)
    - `MaxSequenceNumber` (1 byte): 마지막 조각 번호 (전체 조각 수 - 1)
2.  **데이터 분할 전송**: 네트워크 안정성을 위해 각 UDP 패킷의 크기는 **1000바이트 이내**로 유지해야 합니다. (MTU 및 DTLS 오버헤드 고려)
3.  **DTLS 암호화**: `ImageHeader`를 제외한 실제 JPEG 데이터 조각은 반드시 DTLS로 암호화하여 보내야 합니다.
4.  **피드백 처리**:
    - **유실 대응**: 클라이언트가 TCP 채널로 `{"frame_number": N, "missing_sequence": [X, Y, Z]}`를 보내면, 서버는 해당 조각만 즉시 재송신해야 합니다.
    - **성공 확인**: 클라이언트가 `{"receive": true}`를 보내면 해당 프레임 전송을 종료하고 다음 프레임을 준비합니다.

# MediaDTLS 서버 측 사용 예시

`MediaDTLS.hpp`를 서버에서 사용할 때는 아래와 같은 흐름으로 구현합니다.

```cpp
#include "MediaDTLS.hpp"

// 1. 서버 컨텍스트 및 세션 초기화
SSL_CTX* ctx = MediaDTLS::ServerContext("server.crt", "server.key", "ca.crt");
MediaDTLS::Session session(ctx, true); // true = Server Mode

// 2. UDP 패킷 수신 루프 (가상 코드)
while (true) {
    std::vector<uint8_t> incoming = recv_from_udp();
    
    // [Handshake 단계]
    if (!session.isHandshakeDone()) {
        // 클라이언트의 ClientHello 등을 처리하고 응답 패킷 생성
        auto response = session.Handshake(incoming);
        if (!response.empty()) {
            send_to_udp(response); // 생성된 DTLS 응답 전송
        }
    } 
    // [Data 단계]
    else {
        // 암호화된 이미지 조각 복호화
        auto plain = session.decrypt((const char*)incoming.data(), incoming.size());
        if (!plain.empty()) {
            // 평문(JPEG 조각) 처리 및 필요 시 encrypt() 후 전송
        }
    }
}
```
