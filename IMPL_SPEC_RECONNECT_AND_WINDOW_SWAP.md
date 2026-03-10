# 구현 명세서: 재연결 & CameraWindow Swap

> 작성일: 2026-03-10  
> 대상 프로젝트: `D:\final\Assembly\withBoost`  
> 문서 범위: 아래 두 기능의 **정확한 구현 방법**만 기술한다.  
> 이미 존재하는 코드는 변경 없이 재사용한다.

---

## 목차

- [문제 1 — Online → Offline → Online 재연결](#문제-1--online--offline--online-재연결)
- [문제 2 — CameraWindow Swap 동기화](#문제-2--camerawindow-swap-동기화)

---

## 문제 1 — Online → Offline → Online 재연결

### 1-1. 현재 상황 진단

#### 현재 동작 흐름

```
CAMERA 패킷 수신
  └→ CameraStore::updateFromJson()          [ThreadPool]
       └→ CameraModel::onStoreUpdated()     [GUI thread]
            ├─ isOnline false → true 감지   ✅ 이미 구현됨
            ├─ emit cameraOnline(rtspUrl)   ✅ 이미 구현됨
            └─ emit dataChanged(IsOnlineRole)

Core::wireSignals()
  └→ connect(cameraModel::cameraOnline → videoManager::restartWorker)  ✅ 이미 연결됨

VideoManager::restartWorker(rtspUrl)
  └→ worker->stopStream()
  └→ worker->startStream()                  ← 여기서 문제 발생
```

#### 핵심 문제: `Video::decodeLoopFFmpeg` 에 재시도 루프가 없다

`Video::startStream()` 은 `decodeLoopFFmpeg()` 를 **새 스레드에서 1회 실행**한다.  
연결 실패 또는 스트림 중단 시 `goto cleanup_and_exit` 로 루프가 **완전히 종료**된다.  
이후 `VideoManager::restartWorker` 가 `stopStream()` + `startStream()` 을 호출해도,  
카메라가 아직 완전히 복구되지 않았다면 또다시 1회 시도 후 종료된다.

```cpp
// Video.cpp 현재 코드 — 문제 구간
if (avformat_open_input(...) != 0) {
    fprintf(stderr, "[Video] 입력 오픈 실패\n");
    ...
    goto cleanup_and_exit;   // ← 종료. 재시도 없음.
}

while (!m_stopThread) {
    int ret = av_read_frame(m_formatCtx, packet);
    if (ret >= 0) {
        // 정상 처리
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // ← ret < 0 이어도 루프 계속되지만,
        //   av_read_frame 이 AVERROR_EOF / 네트워크 오류를 반환하면
        //   10ms sleep 후 다시 시도 → tight loop.
        //   단, formatCtx 자체가 이미 파괴된 상태이므로 UB 가능.
    }
}
```

또한 `av_read_frame` 이 네트워크 단절 오류를 반환하기 시작하면  
현재 루프는 **10 ms 간격으로 오류를 무한히 반환**하며 CPU 를 점유한다.  
`goto cleanup_and_exit` 로 빠져나가는 경로가 없기 때문이다.

#### VideoSurface.qml 의 재연결 시도

`VideoManager::restartWorker` 는 `emit workerRegistered(rtspUrl)` 을 호출하지 않는다.  
따라서 `VideoSurface._onRegistered` 는 절대 호출되지 않고,  
`VideoSurface.worker` 는 재시작된 worker 를 재연결하지 못한다.

---

### 1-2. 수정 범위 요약

| 파일 | 변경 유형 | 내용 |
|------|----------|------|
| `Src/Network/Video.cpp` | **수정** | `decodeLoopFFmpeg` 에 외부 재시도 루프 추가 |
| `Src/Config.hpp` | **수정** | `VIDEO_RECONNECT_DELAY_MS` 상수 추가 |
| `Qt/Back/VideoManager.cpp` | **수정** | `restartWorker` 에서 `workerRegistered` emit 추가 |

나머지 파일(`CameraModel`, `Core`, `VideoSurface`) 은 **변경 없음**.  
시그널 연결 (`cameraOnline → restartWorker`) 은 이미 `Core.cpp` 에 존재한다.

---

### 1-3. 수정 1: `Src/Config.hpp`

#### 변경 위치
```
// ── Video ────────────────────────────────────────────────
```
블록 안에 상수 1개 추가.

#### 추가할 코드
```cpp
constexpr int VIDEO_RECONNECT_DELAY_MS = 3000; // 스트림 실패 후 재시도 간격 (ms)
```

#### 변경 후 Video 블록 전체
```cpp
// ── Video ────────────────────────────────────────────────
constexpr int  VIDEO_FPS_LIMIT_MS         = 41;   // ~24 fps
constexpr int  VIDEO_BUFFER_POOL_SIZE_HD  = 8;
constexpr int  VIDEO_BUFFER_POOL_SIZE_4K  = 3;
constexpr int  SPLIT_AUTO_THRESHOLD_WIDTH = 2560;
constexpr int  VIDEO_RECONNECT_DELAY_MS   = 3000; // ← 추가
```

---

### 1-4. 수정 2: `Src/Network/Video.cpp` — `decodeLoopFFmpeg`

#### 목표
`decodeLoopFFmpeg` 의 기존 로직을 내부 함수 `tryOnce()` 로 감싸고,  
외부에 `while (!m_stopThread)` 재시도 루프를 추가한다.  
`tryOnce` 가 실패하면 `VIDEO_RECONNECT_DELAY_MS` (3 초) 대기 후 재시도한다.

#### `av_read_frame` 오류 처리 개선
현재 코드는 `av_read_frame` 실패 시 `sleep(10ms)` 후 계속 루프를 돈다.  
수정 후: **연속 실패 횟수를 카운트**하여 임계값(50회, 약 0.5 초) 초과 시  
`tryOnce` 를 종료(`return false`) → 외부 재시도 루프가 재연결을 담당한다.

#### 수정 전/후 구조 비교

```
// ─── 수정 전 ───────────────────────────────────────────
void Video::decodeLoopFFmpeg(const std::string &url) {
    // ... 초기화 ...
    if (avformat_open_input(...) != 0) { goto cleanup_and_exit; }
    // ... codec 설정 ...
    while (!m_stopThread) {
        int ret = av_read_frame(...);
        if (ret >= 0) { /* 처리 */ }
        else { sleep(10ms); }   // 오류 시 무한 재시도, CPU 낭비
    }
cleanup_and_exit:
    // cleanup
}

// ─── 수정 후 ───────────────────────────────────────────
void Video::decodeLoopFFmpeg(const std::string &url) {
    while (!m_stopThread) {                          // ← 외부 재시도 루프
        bool ok = tryOnceFFmpeg(url);                // ← 1회 연결+디코딩 시도
        cleanupFFmpeg();                             // ← 매 시도 후 반드시 정리
        if (m_stopThread) break;
        if (!ok) {
            fprintf(stderr, "[Video] 재연결 대기 %dms: %s\n",
                    Config::VIDEO_RECONNECT_DELAY_MS, url.c_str());
            // 100ms 단위로 m_stopThread 체크 (빠른 종료 보장)
            for (int i = 0;
                 i < Config::VIDEO_RECONNECT_DELAY_MS / 100 && !m_stopThread;
                 ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

// tryOnceFFmpeg: 기존 decodeLoopFFmpeg 의 본문을 그대로 이동.
// 반환값: true = 정상 종료(m_stopThread), false = 연결/스트림 실패
bool Video::tryOnceFFmpeg(const std::string &url) {
    // ... 기존 초기화 코드 (avformat_open_input 등) ...
    // 연결 실패 시:
    //   return false;   (goto cleanup_and_exit 를 대체)

    int readErrorCount = 0;
    while (!m_stopThread) {
        int ret = av_read_frame(m_formatCtx, packet);
        if (ret >= 0) {
            readErrorCount = 0;
            // ... 기존 프레임 처리 코드 ...
        } else {
            ++readErrorCount;
            if (readErrorCount > 50) {
                fprintf(stderr, "[Video] 스트림 오류 누적, 재연결 트리거: %s\n",
                        url.c_str());
                return false;   // ← 외부 루프가 재연결 처리
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    return true;  // m_stopThread 로 정상 종료
}
```

#### `Video.hpp` 에 추가할 선언
```cpp
private:
    // ...기존 멤버...

    // decodeLoopFFmpeg 의 1회 시도 로직.
    // true  = m_stopThread 에 의한 정상 종료
    // false = 연결 실패 또는 스트림 오류 → 재연결 필요
    bool tryOnceFFmpeg(const std::string &url);
```

#### 전체 `decodeLoopFFmpeg` 교체 코드 (복사 붙여넣기용)

```cpp
// ── Video.cpp: decodeLoopFFmpeg 교체 ──────────────────────────────────────

void Video::decodeLoopFFmpeg(const std::string &url) {
    while (!m_stopThread) {
        bool ok = tryOnceFFmpeg(url);
        cleanupFFmpeg();   // 반드시 매 시도 후 정리 (메모리 누수 방지)

        if (m_stopThread) break;

        if (!ok) {
            fprintf(stderr, "[Video] 스트림 재연결 대기 %dms: %s\n",
                    Config::VIDEO_RECONNECT_DELAY_MS, url.c_str());
            for (int i = 0;
                 i < Config::VIDEO_RECONNECT_DELAY_MS / 100 && !m_stopThread;
                 ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    fprintf(stderr, "[Video] decodeLoopFFmpeg 완전 종료: %s\n", url.c_str());
}

// ── Video.cpp: tryOnceFFmpeg (기존 decodeLoopFFmpeg 본문을 이동) ──────────

bool Video::tryOnceFFmpeg(const std::string &url) {
    AVFrame *frame = nullptr;
    AVFrame *frameRGB = nullptr;
    AVPacket *packet = nullptr;
    AVDictionary *options = nullptr;
    const AVCodec *codec = nullptr;
    int width = 0, height = 0;
    int prevTargetWidth = 0, prevTargetHeight = 0;

    av_log_set_callback(ffmpeg_log_callback);
    av_log_set_level(AV_LOG_WARNING);
    avformat_network_init();

    m_formatCtx = avformat_alloc_context();
    if (!m_formatCtx) {
        fprintf(stderr, "[Video] FFmpeg: 컨텍스트 할당 실패\n");
        return false;
    }

    m_formatCtx->interrupt_callback.callback = [](void *ctx) -> int {
        auto *stopFlag = static_cast<std::atomic<bool> *>(ctx);
        return (*stopFlag) ? 1 : 0;
    };
    m_formatCtx->interrupt_callback.opaque = &m_stopThread;

    m_formatCtx->probesize = 5000000;
    m_formatCtx->max_analyze_duration = 2000000;

    av_dict_set(&options, "rtsp_transport", "udp", 0);
    av_dict_set(&options, "flags", "low_delay", 0);
    av_dict_set(&options, "fflags", "discardcorrupt", 0);
    av_dict_set(&options, "max_delay", "500000", 0);
    av_dict_set(&options, "stimeout", "20000000", 0);
    av_dict_set(&options, "buffer_size", "20000000", 0);

    fprintf(stderr, "[Video] 스트림 연결 시도: %s\n", url.c_str());

    if (avformat_open_input(&m_formatCtx, url.c_str(), nullptr, &options) != 0) {
        fprintf(stderr, "[Video] 입력 오픈 실패: %s\n", url.c_str());
        if (options) av_dict_free(&options);
        avformat_free_context(m_formatCtx);
        m_formatCtx = nullptr;
        return false;   // ← goto cleanup_and_exit 대체
    }
    if (options) av_dict_free(&options);

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0)
        fprintf(stderr, "[Video] 스트림 정보 분석 미흡\n");

    m_videoStreamIndex = -1;
    for (unsigned i = 0; i < m_formatCtx->nb_streams; ++i) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            break;
        }
    }
    if (m_videoStreamIndex == -1) {
        fprintf(stderr, "[Video] 비디오 스트림을 찾을 수 없음\n");
        return false;
    }

    codec = avcodec_find_decoder(
        m_formatCtx->streams[m_videoStreamIndex]->codecpar->codec_id);
    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(
        m_codecCtx, m_formatCtx->streams[m_videoStreamIndex]->codecpar);

    if (m_codecCtx->width <= 0 || m_codecCtx->height <= 0) {
        fprintf(stderr, "[Video] 해상도 미정 — 디코더 강제 초기화\n");
        m_codecCtx->width = 1280;
        m_codecCtx->height = 720;
        m_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        m_codecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }
    m_codecCtx->thread_count = 2;
    m_codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecCtx->skip_loop_filter = AVDISCARD_ALL;
    m_codecCtx->skip_frame = AVDISCARD_DEFAULT;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        fprintf(stderr, "[Video] 코덱 오픈 실패\n");
        return false;
    }

    frame    = av_frame_alloc();
    frameRGB = av_frame_alloc();
    packet   = av_packet_alloc();

    fprintf(stderr, "[Video] 디코딩 루프 시작\n");

    int readErrorCount = 0;  // ← 추가

    while (!m_stopThread) {
        int ret = av_read_frame(m_formatCtx, packet);
        if (ret >= 0) {
            readErrorCount = 0;  // 성공 시 카운터 초기화

            if (packet->stream_index == m_videoStreamIndex) {
                if (avcodec_send_packet(m_codecCtx, packet) == 0) {
                    while (true) {
                        int recv = avcodec_receive_frame(m_codecCtx, frame);
                        if (recv != 0) break;

                        auto now = std::chrono::steady_clock::now();
                        auto elapsed =
                            std::chrono::duration_cast<std::chrono::milliseconds>(
                                now - m_lastUpdateTime).count();

                        if (elapsed >= Config::VIDEO_FPS_LIMIT_MS) {
                            m_lastUpdateTime = now;

                            int srcWidth  = frame->width;
                            int srcHeight = frame->height;
                            if (srcWidth <= 0 || srcHeight <= 0) continue;

                            int targetWidth  = srcWidth;
                            int targetHeight = srcHeight;

                            if (!m_swsCtx || width != srcWidth || height != srcHeight ||
                                prevTargetWidth != targetWidth || prevTargetHeight != targetHeight) {
                                width  = srcWidth;  height = srcHeight;
                                prevTargetWidth = targetWidth; prevTargetHeight = targetHeight;

                                if (m_swsCtx) sws_freeContext(m_swsCtx);

                                int numBytes = av_image_get_buffer_size(
                                    AV_PIX_FMT_RGBA, targetWidth, targetHeight, 4);

                                const int poolSize = (targetWidth > 1920)
                                    ? Config::VIDEO_BUFFER_POOL_SIZE_4K
                                    : Config::VIDEO_BUFFER_POOL_SIZE_HD;

                                m_bufferPool.clear();
                                for (int i = 0; i < poolSize; ++i) {
                                    m_bufferPool.push_back(
                                        std::make_shared<std::vector<uint8_t>>(
                                            numBytes + AV_INPUT_BUFFER_PADDING_SIZE));
                                }

                                m_swsCtx = sws_getContext(
                                    srcWidth, srcHeight,
                                    static_cast<AVPixelFormat>(frame->format),
                                    targetWidth, targetHeight, AV_PIX_FMT_RGBA,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
                            }

                            std::shared_ptr<std::vector<uint8_t>> targetBuffer = nullptr;
                            for (auto &buf : m_bufferPool) {
                                if (buf.use_count() == 1) { targetBuffer = buf; break; }
                            }
                            if (!targetBuffer) continue;

                            av_image_fill_arrays(frameRGB->data, frameRGB->linesize,
                                                 targetBuffer->data(), AV_PIX_FMT_RGBA,
                                                 targetWidth, targetHeight, 4);

                            sws_scale(m_swsCtx, (const uint8_t *const *)frame->data,
                                      frame->linesize, 0, srcHeight,
                                      frameRGB->data, frameRGB->linesize);

                            if (onFrameReady)
                                onFrameReady(targetBuffer, targetWidth, targetHeight,
                                             frameRGB->linesize[0]);
                        }
                    }
                }
            }
            av_packet_unref(packet);

        } else {
            // ── 오류 처리 ─────────────────────────────────────────
            ++readErrorCount;
            if (readErrorCount > 50) {          // ← 약 0.5초 연속 실패
                fprintf(stderr, "[Video] 스트림 오류 누적 (%d회), 재연결 트리거: %s\n",
                        readErrorCount, url.c_str());
                // cleanup 은 호출자(decodeLoopFFmpeg) 가 담당
                if (packet)    av_packet_free(&packet);
                if (frameRGB)  av_frame_free(&frameRGB);
                if (frame)     av_frame_free(&frame);
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    if (packet)    av_packet_free(&packet);
    if (frameRGB)  av_frame_free(&frameRGB);
    if (frame)     av_frame_free(&frame);

    fprintf(stderr, "[Video] 디코딩 루프 정상 종료 (stopThread)\n");
    return true;
}
```

---

### 1-5. 수정 3: `Qt/Back/VideoManager.cpp` — `restartWorker`

#### 현재 코드 (문제)
```cpp
void VideoManager::restartWorker(const QString &rtspUrl) {
    auto it = m_workers.find(rtspUrl);
    if (it == m_workers.end()) { /* 신규 생성 */ return; }
    it.value()->stopStream();
    it.value()->startStream();
    // ← workerRegistered 미발신 → VideoSurface 재연결 불가
}
```

#### 수정 후
```cpp
void VideoManager::restartWorker(const QString &rtspUrl) {
    auto it = m_workers.find(rtspUrl);
    if (it == m_workers.end()) {
        if (rtspUrl.isEmpty()) return;
        qDebug() << "[VideoManager] restartWorker: 새 워커 생성" << rtspUrl;
        auto *worker = new VideoWorker(rtspUrl, this);
        m_workers.insert(rtspUrl, worker);
        worker->startStream();
        emit workerRegistered(rtspUrl);   // VideoSurface._onRegistered 트리거
        return;
    }

    qDebug() << "[VideoManager] restartWorker: 재시작" << rtspUrl;
    VideoWorker *worker = it.value();
    worker->stopStream();
    worker->startStream();
    emit workerRegistered(rtspUrl);       // ← 이 줄 추가
    // VideoSurface 가 이미 worker 를 가지고 있더라도 재발신해도 무해하다.
    // _onRegistered 는 worker = 동일 포인터를 다시 set 하므로 no-op 과 같다.
}
```

#### `emit workerRegistered` 가 이미 연결된 VideoSurface 에게 안전한 이유
```qml
// VideoSurface.qml — _onRegistered
function _onRegistered(url) {
    if (root.slotId >= 0 || root.rtspUrl === url) {
        videoManager.workerRegistered.disconnect(_onRegistered)
        _tryAttach()   // getWorker(url) → worker 재설정
    }
}
```
`_tryAttach()` 는 이미 연결된 worker 에 대해 `root.worker = w` 를 다시 설정한다.  
`VideoSurfaceItem.worker` 는 동일 포인터를 받으므로 렌더링 중단 없음.

---

### 1-6. 재연결 전체 시퀀스 (수정 후)

```
[카메라 네트워크 단절]
  Video::tryOnceFFmpeg
    → av_read_frame 반환값 < 0 카운트 누적 (50회, ~0.5초)
    → return false
  Video::decodeLoopFFmpeg
    → cleanupFFmpeg()
    → 3000ms 대기 (100ms 단위, stopThread 체크)
    → tryOnceFFmpeg 재시도 ... (카메라 복구 전까지 반복)

[서버가 카메라 복구를 감지 → CAMERA 패킷 발신]
  NetworkBridge::cameraListReceived
    → CameraStore::updateFromJson()     [ThreadPool]
    → CameraModel::onStoreUpdated()     [GUI thread]
         isOnline: false → true
         → emit dataChanged(IsOnlineRole)
              → CameraCard.isOnline = true
              → Loader.active = true   (VideoSurface 재생성)
         → emit cameraOnline(rtspUrl)
              → VideoManager::restartWorker(rtspUrl)
                   → worker->stopStream()  (진행 중인 tryOnce 중단)
                   → worker->startStream() (새 스레드에서 tryOnce 즉시 시작)
                   → emit workerRegistered(rtspUrl)
                        → VideoSurface._onRegistered()
                        → VideoSurface._tryAttach()
                        → root.worker = 재시작된 worker

[Video::tryOnceFFmpeg 연결 성공]
  → onFrameReady 콜백 → VideoWorker::m_atomicBuffer 갱신
  → emit frameReady() → VideoSurfaceItem::update()
  → 화면에 영상 출력 재개
```

---

---

## 문제 2 — CameraWindow Swap 동기화

### 2-1. 현재 상황 진단

#### CameraWindow 의 현재 동작 방식

`CameraWindow.qml` 은 `sourceSlotId` 를 기반으로 `cameraModel` 의 데이터를 **실시간 조회**한다.

```qml
// CameraWindow.qml — 현재 코드
property int sourceSlotId: -1

property string rtspUrl: {
    var dummy = updateTrigger;
    if (!cameraModel.hasSlot(root.sourceSlotId)) return sourceRtspUrl;
    return cameraModel.rtspUrlForSlot(root.sourceSlotId);  // ← slotId로 조회
}
```

#### 현재 Swap 동작

`DashboardPage.qml`:
```qml
onSlotSwapped: (sourceIndex, targetIndex) => {
    cameraModel.swapSlots(sourceIndex, targetIndex);  // ← entry 전체 교환
}
```

`CameraModel::swapSlots`:
```cpp
std::swap(m_cameras[indexA], m_cameras[indexB]);
// entry 전체 (slotId, title, rtspUrl, isOnline, cropRect) 를 교환
```

**slotId 도 함께 교환된다.**

#### 문제의 핵심

Swap 후 A 위치에 있던 카드의 `slotId` 가 B 위치로 이동한다.  
`CameraWindow` 는 **자신이 생성될 때 받은 `sourceSlotId`** 를 고정으로 보유한다.  
따라서 Swap 이 일어나도 `sourceSlotId` 는 변하지 않고,  
`cameraModel.rtspUrlForSlot(sourceSlotId)` 는 여전히 **swap 전 카드를 추적**한다.

**이것이 원하는 동작인가?**

요구사항: "CameraWindow 의 주소도 Swap 이 되어야 한다"  
→ Swap 후 CameraWindow 는 **새 위치에 있는 카드의 영상**을 보여줘야 한다.  
→ 즉, CameraWindow 는 **그리드 위치(index) 를 추적**해야 하지, slotId 를 추적하면 안 된다.

#### 현재 동작 예시

```
Swap 전:
  그리드 위치 0: slotId=5, rtsp://A  ← CameraWindow(sourceSlotId=5) 가 이 카드에서 분리됨
  그리드 위치 1: slotId=7, rtsp://B

Swap 후 (swapSlots(0, 1)):
  그리드 위치 0: slotId=7, rtsp://B  ← 여기가 원래 CameraWindow 를 열었던 위치
  그리드 위치 1: slotId=5, rtsp://A

CameraWindow(sourceSlotId=5) 의 rtspUrl:
  = cameraModel.rtspUrlForSlot(5)
  = rtsp://A           ← swap 후에도 여전히 A 를 보여줌 (잘못됨)
  원하는 결과: swap 후 위치 0 의 영상 = rtsp://B
```

---

### 2-2. 설계 결정

`CameraWindow` 가 추적해야 할 것: **그리드 위치(index) 가 아닌 "원본 카드의 slotId"**?

요구사항을 다시 해석한다:

> "Dashboard 에서 Swap 이 되었을때 CameraWindow 의 주소도 Swap 이 되어야한다"

이것은 **CameraWindow 를 연 카드가 Swap 으로 새 위치로 이동할 때,**  
**CameraWindow 도 그 카드를 따라가야 한다**는 의미이다.  
즉 CameraWindow 는 **slotId 가 아니라 그리드 위치(index)** 를 추적해야 한다.

그러나 `swapSlots` 는 **slotId 도 함께 교환**하므로, Swap 후 index 로 조회하는 것과  
slotId 로 조회하는 것이 **반대 결과**를 낳는다.

**올바른 해결책**: `swapSlots` 에서 **slotId 는 교환하지 않고**, rtspUrl / title 등만 교환한다.  
그러면 CameraWindow 는 기존처럼 `sourceSlotId` 로 조회하면 Swap 결과를 자동 반영한다.

---

### 2-3. 설계 변경: `swapSlots` 의 의미론 재정의

#### 현재 의미론 (잘못됨)
```
swapSlots(0, 1):
  index 0 의 entry 전체 ↔ index 1 의 entry 전체
  → slotId 도 교환됨
  → CameraWindow(slotId=5) 는 여전히 A 를 추적
```

#### 새 의미론 (올바름)
```
swapSlots(0, 1):
  index 0 의 slotId 는 유지 (= 5)
  index 0 의 rtspUrl, title, cameraType, cropRect, splitCount 만 교환
  → slotId=5 의 rtspUrl 이 이제 rtsp://B
  → CameraWindow(slotId=5) → rtsp://B 로 자동 전환
```

**slotId = "그리드 위치의 영구 ID"**  
**rtspUrl = "현재 이 위치에서 보여줄 스트림"**  
Swap = 두 위치가 보여주는 스트림을 교환 → slotId 는 유지, rtspUrl 만 교환.

이것이 `VIDEO_STREAMING_SPEC.md` §3 의 본래 설계 의도이기도 하다.

---

### 2-4. 수정 범위 요약

| 파일 | 변경 유형 | 내용 |
|------|----------|------|
| `Qt/Back/CameraModel.cpp` | **수정** | `swapSlots`: slotId/splitCount/cropRect/splitGroupId/splitIndex 는 유지, 나머지만 교환 |
| `Qt/Front/Component/camera/CameraWindow.qml` | **변경 없음** | 이미 slotId 로 실시간 조회 중 → 수정 후 자동 동작 |
| `Qt/Back/VideoSurfaceItem` | **변경 없음** | slotId 기반 lookup 은 이미 올바름 |

---

### 2-5. 수정: `Qt/Back/CameraModel.cpp` — `swapSlots`

#### 교환 대상 필드 정의

| 필드 | 교환 여부 | 이유 |
|------|----------|------|
| `slotId` | ❌ 유지 | 그리드 위치의 영구 ID — CameraWindow 가 추적 |
| `rtspUrl` | ✅ 교환 | 이 위치에서 보여줄 스트림 |
| `title` | ✅ 교환 | rtspUrl 에 종속된 표시 이름 |
| `cameraType` | ✅ 교환 | HANWHA / SUB_PI — 스트림에 종속 |
| `isOnline` | ✅ 교환 | 현재 스트림 상태 |
| `description` | ✅ 교환 | 스트림 메타데이터 |
| `width`, `height` | ✅ 교환 | 스트림 해상도 힌트 |
| `splitCount` | ❌ 유지 | 이 위치의 타일 분할 설정 |
| `cropRect` | ❌ 유지 | 이 위치의 UV 크롭 |
| `splitGroupId` | ❌ 유지 | 이 위치의 타일 그룹 ID |
| `splitIndex` | ❌ 유지 | 이 위치의 타일 인덱스 |

#### 수정 전 코드
```cpp
void CameraModel::swapSlots(int indexA, int indexB) {
    if (indexA < 0 || indexB < 0 || indexA >= m_cameras.size() ||
        indexB >= m_cameras.size() || indexA == indexB)
        return;

    std::swap(m_cameras[indexA], m_cameras[indexB]);   // ← 전체 교환 (slotId 포함)

    const auto idxA = createIndex(indexA, 0);
    const auto idxB = createIndex(indexB, 0);
    emit dataChanged(idxA, idxA, {SlotIdRole, TitleRole, RtspUrlRole, ...});
    emit dataChanged(idxB, idxB, {SlotIdRole, TitleRole, RtspUrlRole, ...});
}
```

#### 수정 후 코드
```cpp
void CameraModel::swapSlots(int indexA, int indexB) {
    if (indexA < 0 || indexB < 0 || indexA >= m_cameras.size() ||
        indexB >= m_cameras.size() || indexA == indexB)
        return;

    CameraEntry &a = m_cameras[indexA];
    CameraEntry &b = m_cameras[indexB];

    // ── 스트림 관련 필드만 교환 (위치 관련 필드는 유지) ──────────────────
    // 교환: 이 위치에서 보여줄 스트림 정보
    std::swap(a.rtspUrl,     b.rtspUrl);
    std::swap(a.title,       b.title);
    std::swap(a.cameraType,  b.cameraType);
    std::swap(a.isOnline,    b.isOnline);
    std::swap(a.description, b.description);
    std::swap(a.width,       b.width);
    std::swap(a.height,      b.height);
    // 유지: slotId, splitCount, cropRect, splitGroupId, splitIndex

    // ── 변경된 role 만 명시적으로 통지 ──────────────────────────────────
    const QList<int> changedRoles = {
        TitleRole, RtspUrlRole, IsOnlineRole,
        DescriptionRole, CardWidthRole, CardHeightRole, CameraTypeRole
        // SlotIdRole, SplitCountRole, CropRectRole 은 변경되지 않음
    };

    emit dataChanged(createIndex(indexA, 0), createIndex(indexA, 0), changedRoles);
    emit dataChanged(createIndex(indexB, 0), createIndex(indexB, 0), changedRoles);

    // ── VideoManager 슬롯 맵 동기화 ──────────────────────────────────────
    // slotId 는 유지되지만 rtspUrl 이 교환되었으므로
    // m_slotToUrl 도 갱신해야 getWorkerBySlot 이 올바른 worker 를 반환한다.
    _emitSlotsUpdated();
}
```

---

### 2-6. Swap 후 전체 동작 시퀀스 (수정 후)

```
초기 상태:
  m_cameras[0]: slotId=5, rtspUrl=rtsp://A, title="Cam A"
  m_cameras[1]: slotId=7, rtspUrl=rtsp://B, title="Cam B"
  
  VideoManager::m_slotToUrl: { 5→"rtsp://A", 7→"rtsp://B" }

  CameraWindow(sourceSlotId=5):
    rtspUrl = cameraModel.rtspUrlForSlot(5) = "rtsp://A"  ← A 스트림 표시

─────────────────────────────────────────────────────────
사용자가 카드 A 를 카드 B 위치로 드래그 & 드롭
─────────────────────────────────────────────────────────

DashboardPage.onSlotSwapped(0, 1)
  → cameraModel.swapSlots(0, 1)

CameraModel::swapSlots(0, 1):
  교환: rtspUrl, title, cameraType, isOnline, ...
  유지: slotId (0번=5, 1번=7 유지)

  m_cameras[0] 후: slotId=5, rtspUrl=rtsp://B, title="Cam B"
  m_cameras[1] 후: slotId=7, rtspUrl=rtsp://A, title="Cam A"

  emit dataChanged(index 0, {TitleRole, RtspUrlRole, ...})
  emit dataChanged(index 1, {TitleRole, RtspUrlRole, ...})
  _emitSlotsUpdated()
    → emit slotsUpdated([{slotId=5, url="rtsp://B"}, {slotId=7, url="rtsp://A"}])
    → VideoManager::registerSlots()
         m_slotToUrl 갱신: { 5→"rtsp://B", 7→"rtsp://A" }

그리드 UI:
  위치 0 카드: slotId=5, rtspUrl="rtsp://B" → B 스트림 표시 ✅
  위치 1 카드: slotId=7, rtspUrl="rtsp://A" → A 스트림 표시 ✅

CameraWindow(sourceSlotId=5):
  updateTrigger++ (dataChanged 감지)
  rtspUrl = cameraModel.rtspUrlForSlot(5) = "rtsp://B"  ← 자동 갱신 ✅
  VideoSurface: slotId=5
    → videoManager.getWorkerBySlot(5)
    → m_slotToUrl[5] = "rtsp://B"
    → getWorker("rtsp://B") → B 워커 반환
    → B 스트림 렌더링 ✅
```

---

### 2-7. VideoSurface 재연결 없이 전환되는 이유

`VideoSurface.qml` 은 `slotId` 기반으로 worker 를 조회한다:
```qml
// VideoSurface.qml
onSlotIdChanged: { _tryAttach() }   // slotId 가 바뀔 때만 재연결
```

Swap 후 `slotId` 는 변하지 않는다 (= 5 유지).  
따라서 `onSlotIdChanged` 는 발생하지 않는다.

단, `rtspUrl` 이 바뀌었으므로 `VideoManager::m_slotToUrl[5]` 가 갱신된 후  
다음 프레임 요청 시 `getWorkerBySlot(5)` 는 새 URL 의 worker 를 반환한다.

`VideoSurfaceItem` 은 `worker` 프로퍼티가 변경될 때 자동으로 새 worker 의  
`frameReady` 시그널에 연결하므로 **영상 전환이 부드럽게 이루어진다**.

---

## 전체 변경 파일 요약

| # | 파일 | 변경 내용 |
|---|------|----------|
| 1 | `Src/Config.hpp` | `VIDEO_RECONNECT_DELAY_MS = 3000` 추가 |
| 2 | `Src/Network/Video.hpp` | `bool tryOnceFFmpeg(const std::string &url)` 선언 추가 |
| 3 | `Src/Network/Video.cpp` | `decodeLoopFFmpeg` → 재시도 루프로 교체; `tryOnceFFmpeg` 구현 추가 |
| 4 | `Qt/Back/VideoManager.cpp` | `restartWorker` 끝에 `emit workerRegistered(rtspUrl)` 추가 |
| 5 | `Qt/Back/CameraModel.cpp` | `swapSlots`: slotId 유지, 스트림 필드만 교환 |

변경 없는 파일 (기존 코드가 이미 올바름):
- `CameraModel.hpp` (signal `cameraOnline` 이미 있음)
- `Core.cpp` (wiring 이미 있음)
- `VideoSurface.qml` (slotId 기반 조회 이미 올바름)
- `CameraWindow.qml` (slotId 로 실시간 조회 이미 올바름)
