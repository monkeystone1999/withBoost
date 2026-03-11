#include "Video.hpp"
#include "../Config.hpp"
#include <chrono>
#include <cstdio>

static void ffmpeg_log_callback(void *, int level, const char *fmt,
                                va_list vl) {
  if (level > AV_LOG_WARNING)
    return;
  char buf[1024];
  vsnprintf(buf, sizeof(buf), fmt, vl);
  // 개행 제거 후 stderr 출력
  int len = static_cast<int>(strlen(buf));
  while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
    buf[--len] = '\0';
  if (len > 0)
    fprintf(stderr, "[FFmpeg] %s\n", buf);
}

Video::~Video() { stopStream(); }

void Video::startStream(const std::string &url) {
  if (url.empty())
    return;

  stopStream();

  m_stopThread = false;
  m_decodeThread = std::thread([this, url]() { decodeLoopFFmpeg(url); });
}

void Video::stopStream() {
  m_stopThread = true;
  if (m_decodeThread.joinable()) {
    m_decodeThread.join();
  }
  cleanupFFmpeg();
}

void Video::decodeLoopFFmpeg(const std::string &url) {
  // Retry loop
  while (!m_stopThread) {
    bool ok = tryOnceFFmpeg(url);
    if (m_stopThread)
      break;
    if (!ok) {
      fprintf(stderr, "[Video] 연결 실패/끊김 — 3초 후 재시도: %s\n",
              url.c_str());
      for (int i = 0; i < 30 && !m_stopThread; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  fprintf(stderr, "[Video] 디코딩 루프 종료\n");
}

bool Video::tryOnceFFmpeg(const std::string &url) {
  AVFrame *frame = nullptr;
  AVFrame *frameRGB = nullptr;
  AVPacket *packet = nullptr;
  AVDictionary *options = nullptr;
  const AVCodec *codec = nullptr;
  int width = 0, height = 0;
  int prevTargetWidth = 0, prevTargetHeight = 0;
  bool success = true; // false → caller retries

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
    if (options)
      av_dict_free(&options);
    avformat_free_context(m_formatCtx);
    m_formatCtx = nullptr;
    return false;
  }
  if (options)
    av_dict_free(&options);

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
    cleanupFFmpeg();
    return false;
  }

  codec = avcodec_find_decoder(
      m_formatCtx->streams[m_videoStreamIndex]->codecpar->codec_id);
  m_codecCtx = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(
      m_codecCtx, m_formatCtx->streams[m_videoStreamIndex]->codecpar);

  if (m_codecCtx->width <= 0 || m_codecCtx->height <= 0) {
    fprintf(stderr, "[Video] Resolution unknown - decoding as is\n");
  }
  m_codecCtx->thread_count = 0;
  m_codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
  m_codecCtx->skip_loop_filter = AVDISCARD_ALL;
  m_codecCtx->skip_frame = AVDISCARD_DEFAULT;

  if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
    fprintf(stderr, "[Video] 코덱 오픈 실패\n");
    cleanupFFmpeg();
    return false;
  }

  frame = av_frame_alloc();
  frameRGB = av_frame_alloc();
  packet = av_packet_alloc();

  fprintf(stderr, "[Video] 디코딩 시작\n");

  int consecutiveErrors = 0;
  constexpr int kMaxErrors = 50; // ~0.5초 (10ms × 50)

  while (!m_stopThread) {
    int ret = av_read_frame(m_formatCtx, packet);
    if (ret >= 0) {
      consecutiveErrors = 0;
      if (packet->stream_index == m_videoStreamIndex) {
        if (avcodec_send_packet(m_codecCtx, packet) == 0) {
          while (true) {
            int recv = avcodec_receive_frame(m_codecCtx, frame);
            if (recv != 0)
              break;

            // ──────────────────────────────────────────────────────────────
            // FPS 제한 제거 (실시간 스트리밍 최적화)
            //
            // 이전: Config::VIDEO_FPS_LIMIT_MS (41ms) 마다 프레임 처리
            // 현재: 모든 프레임을 즉시 처리하여 VideoWorker로 전달
            //
            // 이유:
            // 1. VideoSurfaceItem의 beforeSynchronizing이 렌더링 주기
            //    (~16.67ms for 60Hz)에 맞춰 자동으로 throttling
            // 2. VideoWorker의 atomic 덮어쓰기로 중간 프레임 자동 스킵
            // 3. GUI Thread Sync 시간 영향 없음 (atomic read는 ~수십ns)
            // 4. FFmpeg 디코딩 지연 최소화로 실시간성 향상
            // ──────────────────────────────────────────────────────────────

            int srcWidth = frame->width;
            int srcHeight = frame->height;
            if (srcWidth <= 0 || srcHeight <= 0)
              continue;

            int targetWidth = srcWidth;
            int targetHeight = srcHeight;

            if (!m_swsCtx || width != srcWidth || height != srcHeight ||
                prevTargetWidth != targetWidth ||
                prevTargetHeight != targetHeight) {
              width = srcWidth;
              height = srcHeight;
              prevTargetWidth = targetWidth;
              prevTargetHeight = targetHeight;

              if (m_swsCtx)
                sws_freeContext(m_swsCtx);

              int numBytes = av_image_get_buffer_size(
                  AV_PIX_FMT_RGBA, targetWidth, targetHeight, 4);

              const int poolSize = (targetWidth > 1920)
                                       ? Config::VIDEO_BUFFER_POOL_SIZE_4K
                                       : Config::VIDEO_BUFFER_POOL_SIZE_HD;

              m_bufferPool.clear();
              for (int i = 0; i < poolSize; ++i) {
                auto buf = std::make_shared<std::vector<uint8_t>>(
                    numBytes + AV_INPUT_BUFFER_PADDING_SIZE);
                m_bufferPool.push_back(buf);
              }

              m_swsCtx =
                  sws_getContext(srcWidth, srcHeight,
                                 static_cast<AVPixelFormat>(frame->format),
                                 targetWidth, targetHeight, AV_PIX_FMT_RGBA,
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
            }

            std::shared_ptr<std::vector<uint8_t>> targetBuffer = nullptr;
            for (auto &buf : m_bufferPool) {
              if (buf.use_count() == 1) {
                targetBuffer = buf;
                break;
              }
            }
            if (!targetBuffer)
              continue;

            av_image_fill_arrays(frameRGB->data, frameRGB->linesize,
                                 targetBuffer->data(), AV_PIX_FMT_RGBA,
                                 targetWidth, targetHeight, 4);

            sws_scale(m_swsCtx, (const uint8_t *const *)frame->data,
                      frame->linesize, 0, srcHeight, frameRGB->data,
                      frameRGB->linesize);

            if (onFrameReady) {
              onFrameReady(targetBuffer, targetWidth, targetHeight,
                           frameRGB->linesize[0]);
            }
          }
        }
      }
      av_packet_unref(packet);
    } else {
      // av_read_frame 실패: 연속 오류가 kMaxErrors 에 달하면 재연결 시도
      ++consecutiveErrors;
      if (consecutiveErrors >= kMaxErrors) {
        fprintf(stderr, "[Video] 연속 %d 회 read 실패 — 재연결\n", kMaxErrors);
        success = false;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  if (packet)
    av_packet_free(&packet);
  if (frameRGB)
    av_frame_free(&frameRGB);
  if (frame)
    av_frame_free(&frame);
  cleanupFFmpeg();
  return success;
}

void Video::cleanupFFmpeg() {
  if (m_swsCtx) {
    sws_freeContext(m_swsCtx);
    m_swsCtx = nullptr;
  }
  if (m_codecCtx) {
    avcodec_free_context(&m_codecCtx);
    m_codecCtx = nullptr;
  }
  if (m_formatCtx) {
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
  }
  m_videoStreamIndex = -1;
}
