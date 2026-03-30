#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

/**
 * @file VideoEngine.hpp
 * @brief FFmpeg 기반 비디오 디코딩 엔진
 *
 * 이 파일은 RTSP 등 네트워크 비디오 스트림을 수신하고 디코딩하는 기능을
 * 제공합니다. FFmpeg 라이브러리를 사용하여 하드웨어 가속(D3D11VA) 디코딩을
 * 지원하며, 별도의 작업 스레드에서 루프를 수행하여 메인 스레드의 블로킹을
 * 방지합니다.
 */

/**
 * @class VideoEngine
 * @brief 실시간 비디오 스트림 디코더 및 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. startStream()을 호출하여 RTSP URL과 FPS 제한 설정을 초기화하고 디코딩
 * 스레드를 실행합니다.
 * 2. onFrameReady 콜백을 설정하여 디코딩이 완료된 RAW 프레임(NV12)을
 * 처리합니다.
 * 3. 더 이상 스트림이 필요하지 않으면 stopStream()을 호출하여 리소스를
 * 정리합니다.
 */
class VideoEngine {
public:
  /**
   * @struct FramePayload
   * @brief 디코딩이 완료된 프레임 데이터 구조체
   */
  struct FramePayload {
    const uint8_t *dataY = nullptr; /**< Y 평면 데이터 주소 */
    const uint8_t *dataUV =
        nullptr;      /**< UV(크로마) 평면 데이터 주소 (NV12 기준) */
    int width = 0;    /**< 프레임 가로 해상도 */
    int height = 0;   /**< 프레임 세로 해상도 */
    int strideY = 0;  /**< Y 데이터의 라인별 바이트 수 (Pitch) */
    int strideUV = 0; /**< UV 데이터의 라인별 바이트 수 */
  };

  /** @brief 새 프레임이 준비되었을 때 호출될 콜백 함수 타입 */
  using FrameCallback = std::function<void(const FramePayload &)>;

  explicit VideoEngine() = default;
  ~VideoEngine();

  /**
   * @brief 스트림 디코딩 시작
   * @param url RTSP/RTMP 등 비디오 소스 주소
   * @param fpsLimit 출력할 최대 프레임 수 (초당)
   * @note 내부적으로 전용 스레드를 생성하여 루프를 시작하며, 실패 시 3초마다
   * 재시도를 수행합니다.
   */
  void startStream(const std::string &url, int fpsLimit);

  /** @param fps 초당 프레임 수 제한값 업데이트 */
  void setFpsLimit(int fps);

  /** @brief 디코딩 스레드 정지 및 FFmpeg 컨텍스트 폐기 */
  void stopStream();

  /** @brief 디코딩된 프레임을 외부로 전달하는 함수 객체 */
  FrameCallback onFrameReady;

private:
  void decodeLoopFFmpeg(const std::string &url);
  bool tryOnceFFmpeg(const std::string &url);
  void cleanupFFmpeg();
  static int interrupt_cb(void *ctx);

  AVFormatContext *formatCtx_ = nullptr;
  AVCodecContext *codecCtx_ = nullptr;
  int videoStreamIndex_ = -1;

  std::thread decodeThread_;
  std::atomic<bool> stopThread_{false};
  std::chrono::steady_clock::time_point lastFrameTime_;
  std::atomic<int> fpsLimitMs_{33};

  bool isStopping() const {
    return stopThread_.load(std::memory_order_relaxed);
  }
};

/**
 * @section Workflow Guide
 *
 * **[VideoEngine 통합 가이드]**
 *
 * 1. 렌더러 연결:
 *    - UI 레이어의 비디오 위젯은 `VideoEngine::onFrameReady`에 자신의 렌더링
 * 함수를 바인딩합니다.
 *    - 이때 전달되는 데이터는 GPU 메모리에서 호스트 메모리로 복사된 NV12
 * 데이터입니다.
 *
 * 2. 네트워크 연결 유지:
 *    - `decodeLoopFFmpeg`는 네트워크 단절 시 자동으로 재연결을 시도합니다.
 *    - FFmpeg의 `interrupt_callback`을 통해 `stopStream()` 호출 시 블로킹 중인
 * I/O 작업을 즉시 중단합니다.
 *
 * 3. 성능 최적화:
 *    - `setFpsLimit`을 통해 CPU/GPU 부하를 동적으로 조절할 수 있습니다.
 *    - 하드웨어 가속(D3D11VA)이 활성화되어 있어 저사양 환경에서도 원활한
 * 디코딩을 지원합니다.
 */

/// 호환성을 위한 타입 에일리어스
using Video = VideoEngine;
