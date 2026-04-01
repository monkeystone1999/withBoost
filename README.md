# AnoMap

보안 카메라 모니터링 데스크탑 애플리케이션 (Qt6 / C++20 / Windows)

---

## 아키텍처 개요

```
QML (Front)
  │  props / signals
  ▼
Layer 2 — QObject 어댑터  (Qt/Back)
  │  Qt::QueuedConnection
  ▼
Layer 1 — 순수 C++ 도메인 (Src/)
  │  boost::asio + TLS/DTLS
  ▼
서버 (TCP TLS 20000 / UDP DTLS)
```

전체 객체 수명과 의존성 주입은 `Core.cpp` 한 곳에서 담당합니다.  
`App.cpp` 는 Qt 엔진 설정과 `Core::init()` 호출만 합니다.

---

## 레이어별 역할

### Layer 1 — 순수 C++ (`Src/`)

Qt에 의존하지 않는 순수 C++ 클래스들. `Core`가 `unique_ptr`로 소유합니다.

> **네트워크 흐름**  
> TCP TLS 세션이 패킷을 수신 → `MessageProcessor`가 MessageType 분류 →  
> `NetworkBridge`(Layer 2)가 Qt 시그널로 변환 → `Core::wireSignals()`에서  
> `ThreadPool`을 통해 파싱 → `QMetaObject::invokeMethod`로 GUI 스레드 모델 갱신

### Layer 2 — QObject 어댑터 (`Qt/Back/`)

`QQmlEngine`이 소유(parented). `Core`는 raw pointer만 보관합니다.

### QML Front (`Qt/Front/`)

---

## 네트워크 프로토콜 (MessageType)

### 서버 → 클라이언트 (수신)

| MessageType | 값 | 처리 경로 |
|---|---|---|
| `LOGIN` | `0x01` | `LoginController` → 로그인 성공/실패 시그널 |
| `CAMERA` | `0x04` | `ThreadPool` → `CameraStore` + `DeviceStore` → `CameraModel`, `DeviceModel` |
| `AVAILABLE` | `0x05` | `ThreadPool` → `DeviceStore` + `ServerStatusStore` → `DeviceModel`, `serverStatus` |
| `AI` | `0x06` | `AlarmDispatcher` → `AlarmController` / `CameraStore` person_count |
| `META` | `0x09` | `ThreadPool` → `CameraStore` sensor_batch → `CameraModel` 센서 갱신 |
| `IMAGE` | `0x0A` | `ThreadPool` → Base64 변환 → `AiImageModel` |

### 클라이언트 → 서버 (송신)

`NetworkBridge::send()` (또는 `INetworkService::send()`) 를 통해 전송합니다.

| 용도 | MessageType |
|---|---|
| 로그인 | `LOGIN (0x01)` |
| 회원가입/승인 | `ASSIGN (0x08)` |
| UDP 포트 알림 | `IMAGE (0x0A)` |
| 손실 프레임 재전송 요청 | `IMAGE (0x0A)` + loss 정보 |

### UDP 이미지 스트림 흐름

```
서버: IMAGE{이미지 메타} → 클라이언트: IMAGE{UDP 포트 번호}
                         ← 이미지 수신 중
클라이언트: IMAGE{loss된 frame/sequence 번호} → 재전송 요청
```

---

## 카메라 카드 UI 기능

`CameraModel` (`Qt/Back/Models/CameraModel.cpp`)이 슬롯 그리드를 관리합니다.

| 기능 | 메서드 | 동작 |
|---|---|---|
| **Swap** | `swapSlots(indexA, indexB)` | 컨텐츠만 교환, `slotId` 유지 → VideoSurface 연결 끊김 없음 |
| **Split** | `splitSlot(index, n, direction)` | 단일 슬롯을 n개 타일로 분리, 각 타일에 CropRect(UV 좌표) 할당 |
| **Merge** | `mergeSlots(index, n)` | 같은 `splitGroupId` 타일을 하나로 통합 |

분할 방향: `0` = Col(수직), `1` = Row(수평), `2` = Grid(2×2)

---

## 배포 설정

`Src/Config.hpp` 에서 컴파일 타임 상수를 수정합니다.

```cpp
// 서버 주소
constexpr std::string_view SERVER_HOST = "192.168.0.58";
constexpr std::string_view SERVER_PORT = "20000";

// 카메라 ID별 자동 분할 설정 (배포 환경에 맞게 채워 넣기)
constexpr std::array<AutoSplitEntry, 0> AUTO_SPLIT_CAMERA_ID_TILE_MAP{};
```

---

## 빌드

### 사전 요구사항

- Windows 10/11 (x64)
- Qt 6.8+
- vcpkg (`VCPKG_ROOT` 환경변수 설정 필요)
- Ninja

vcpkg 의존성: `boost-asio`, `openssl`, `nlohmann-json`, `ffmpeg`, `palsigslot`, `libyuv`

### 빌드 명령

```bash
# Configure (Debug)
cmake --preset anomap-ninja-debug

# Build
cmake --build --preset anomap-ninja-debug

# Configure (Release)
cmake --preset anomap-ninja-release
cmake --build --preset anomap-ninja-release
```

---

## 핵심 설계 원칙

- **모든 크로스 스레드 콜백은 반드시 `Qt::QueuedConnection` 또는 `QMetaObject::invokeMethod`를 사용**
- **Dumb Component 원칙**: `components/` 파일은 C++ 컨텍스트 객체를 직접 참조하지 않음
- **Layer 1은 Qt 헤더를 포함하지 않음** — 순수 C++ 도메인 격리

---