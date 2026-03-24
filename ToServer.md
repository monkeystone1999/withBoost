# Anomap Custom TLS Protocol Specification (For Server)

본 문서는 클라이언트(Qt)와 서버 간의 **커스텀 TLS 통신 규격**을 명세합니다.
서버 개발자는 반드시 아래의 규격에 따라 TCP 레이어와 TLS(OpenSSL) 레이어를 연동해야 합니다.

## 1. 기본 통신 규격 (DDoS 방어용 5바이트 평문 헤더)

모든 TCP 패킷은 예외 없이 **`[5바이트 평문 헤더] + [N바이트 페이로드]`** 구조를 가집니다.

| 필드 | 크기 | 타입 | 설명 |
| :--- | :--- | :--- | :--- |
| **Type** | 1 byte | `uint8_t` | 메시지의 종류 (DDoS 필터링 및 라우팅 목적용 평문) |
| **Length** | 4 byte | `uint32_t` | 뒤따라오는 **페이로드의 크기 (N)** (Little-Endian) |
| **Payload** | N byte | `uint8_t[]` | 암호화된 애플리케이션 데이터 또는 TLS 핸드셰이크 데이터 |

- 클라이언트가 보내는 모든 데이터의 `Type` 값은 `Protocol.hpp`에 정의된 `MessageType` 열거형을 따릅니다.
- **[중요]** 헤더 5바이트 자체는 **절대 암호화되지 않습니다.** 이를 통해 서버 측 네트워크 앞단(또는 패킷 수신 직후)에서 비정상적인 `Type` 값이 들어오면 OpenSSL 연산 전에 즉시 소켓을 끊어버리는(Drop) 방어가 가능합니다.

---

## 2. TLS 초기 연결 (Handshake) 처리 방법

TLS Handshake(Client Hello, Server Hello 등) 역시 소켓을 통해 전달될 때는 **반드시 5바이트 헤더로 감싸져(Wrapped)** 전송됩니다. 이때의 `Type` 값은 **`0x11 (TLS_HANDSHAKE)`** 을 사용합니다.

### 서버 측 동작 시나리오 (Handshake)
1. 클라이언트 접속 수락 후 소켓에서 **5바이트**를 파싱합니다.
2. `Type == 0x11` 이고 `Length == N` 임을 확인합니다.
3. 소켓에서 `N`바이트를 마저 읽어들입니다. 이 바이트들은 OpenSSL이 생성한 순수한 TLS Handshake 레코드(Client Hello)입니다.
4. 이 `N`바이트를 OpenSSL 메모리 BIO(`readBio`)에 밀어넣고(`BIO_write`), `SSL_do_handshake()` 내지는 `SSL_read()`를 호출합니다.
5. 서버 측 OpenSSL이 응답 데이터(Server Hello, Certificate 등)를 `writeBio`에 뱉어냅니다 (`BIO_pending` 확인).
6. 뱉어낸 응답 데이터를 꺼내서(`BIO_read`), 그 길이를 측정(M바이트)합니다.
7. 똑같이 **앞에 5바이트 헤더(`Type = 0x11`, `Length = M`)**를 씌운 뒤 클라이언트에게 송신(`TCP Send`)합니다.
8. `SSL_is_init_finished(ssl)` 가 참이 될 때까지 이 과정을 반복합니다.

---

## 3. 애플리케이션 데이터 (Application Data) 처리 방법

Handshake가 무사히 종료된 후 교환되는 모든 데이터(LOGIN, IMAGE, META 등)는 **Payload 부분이 TLS로 암호화(Ciphertext)되어** 전송됩니다.

### 서버 측 분해 (TCP Receive -> Decrypt)
1. 소켓에서 **5바이트**를 파싱합니다. (예: `Type = 0x01 (LOGIN)`, `Length = 64`)
2. 소켓에서 64바이트를 읽습니다. 이는 평문이 아닌 **암호문(Ciphertext)**입니다.
3. 64바이트를 OpenSSL `readBio`에 쓰기(`BIO_write`) 하고, `SSL_read()`를 통해 복호화된 **평문(Plaintext)**을 꺼냅니다.
4. 그 평문 데이터를 어플리케이션(비즈니스 로직)으로 넘겨 처리합니다. (이때 비즈니스 로직은 `Type = 0x01` 이라는 정보와 복호화된 평문만을 봅니다).

### 서버 측 포장 (Encrypt -> TCP Send)
1. 서버 비즈니스 로직이 특정 모듈로 보낼 평문 응답(예: 20바이트)과 응답 Type(예: `0x02 (SUCCESS)`)을 결정합니다.
2. 20바이트 평문 데이터를 OpenSSL `SSL_write()`에 집어넣습니다.
3. OpenSSL 내부에서 암호화를 거쳐 `writeBio`에 암호문(예: 45바이트)을 뱉어냅니다.
4. 이 45바이트 암호문에 **5바이트 헤더(`Type = 0x02`, `Length = 45`)**를 씌웁니다.
5. 완성된 50바이트 패킷을 클라이언트에게 송신(`TCP Send`)합니다.

---

## 4. 요약 및 주의사항

- **TLS 의존성 분리**: `TcpSession` 등 네트워크 최하단 클래스가 소켓 I/O를 담당하고, OpenSSL 연산은 메모리 BIO(Memory BIO)를 사용하여 별도로 수행할 것을 강력히 권장합니다.
- **예외 처리**: 만약 `Type`이 미리 정의된 0x01 ~ 0x0A, 0x11 범위를 벗어나거나, `Length`가 서버가 감당할 수 없을 정도로 크다면 **데이터를 읽지 않고 즉각 접속을 해제**해야 DDoS 공격을 방어할 수 있습니다.
- **연결 유지**: `TLS_HANDSHAKE(0x11)` 메시지를 수신했을 경우, 복호화 파이프라인(BIO) 구동 후 어플리케이션 핸들러로 데이터를 올려보내지 말고 곧바로 다음 5바이트 헤더 읽기 모드로 넘어가야 합니다.


