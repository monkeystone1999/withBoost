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

  stopThread_ = false;
  decodeThread_ = std::thread([this, url]() { decodeLoopFFmpeg(url); });
}

void Video::stopStream() {
  stopThread_ = true;
  if (decodeThread_.joinable()) {
    decodeThread_.join();
  }
  cleanupFFmpeg();
}

void Video::decodeLoopFFmpeg(const std::string &url) {
  // Retry loop
  while (!stopThread_) {
    bool ok = tryOnceFFmpeg(url);
    if (stopThread_)
      break;
    if (!ok) {
      fprintf(stderr, "[Video] 연결 실패/끊김 — 3초 후 재시도: %s\n",
              url.c_str());
      for (int i = 0; i < 30 && !stopThread_; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  fprintf(stderr, "[Video] 디코딩 루프 종료\n");
}

bool Video::tryOnceFFmpeg(const std::string &url) {
    AVFrame *frame = nullptr;
    AVFrame *frameRGB = nullptr;
    AVFrame *swFrame = av_frame_alloc(); // 🚀 HW 프레임을 다운로드할 임시 프레임 추가
    AVPacket *packet = nullptr;
    AVDictionary *options = nullptr;
    const AVCodec *codec = nullptr;
    int width = 0, height = 0;
    int prevTargetWidth = 0, prevTargetHeight = 0;
    bool success = true;

    av_log_set_callback(ffmpeg_log_callback);
    av_log_set_level(AV_LOG_WARNING);
    avformat_network_init();

    formatCtx_ = avformat_alloc_context();
    if (!formatCtx_) {
        fprintf(stderr, "[Video] FFmpeg: 컨텍스트 할당 실패\n");
        return false;
    }

    formatCtx_->interrupt_callback.callback = [](void *ctx) -> int {
        auto *stopFlag = static_cast<std::atomic<bool> *>(ctx);
        return (*stopFlag) ? 1 : 0;
    };
    formatCtx_->interrupt_callback.opaque = &stopThread_;

    formatCtx_->probesize = 5000000;
    formatCtx_->max_analyze_duration = 2000000;

    av_dict_set(&options, "rtsp_transport", "udp", 0);
    av_dict_set(&options, "flags", "low_delay", 0);
    av_dict_set(&options, "fflags", "discardcorrupt", 0);
    av_dict_set(&options, "max_delay", "500000", 0);
    av_dict_set(&options, "stimeout", "20000000", 0);
    av_dict_set(&options, "buffer_size", "20000000", 0);

    fprintf(stderr, "[Video] 스트림 연결 시도: %s\n", url.c_str());

    if (avformat_open_input(&formatCtx_, url.c_str(), nullptr, &options) != 0) {
        fprintf(stderr, "[Video] 입력 오픈 실패: %s\n", url.c_str());
        if (options)
            av_dict_free(&options);
        avformat_free_context(formatCtx_);
        formatCtx_ = nullptr;
        return false;
    }
    if (options)
        av_dict_free(&options);

    if (avformat_find_stream_info(formatCtx_, nullptr) < 0)
        fprintf(stderr, "[Video] 스트림 정보 분석 미흡\n");

    videoStreamIndex_ = -1;
    for (unsigned i = 0; i < formatCtx_->nb_streams; ++i) {
        if (formatCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
            break;
        }
    }
    if (videoStreamIndex_ == -1) {
        fprintf(stderr, "[Video] 비디오 스트림을 찾을 수 없음\n");
        cleanupFFmpeg();
        return false;
    }

    codec = avcodec_find_decoder(
        formatCtx_->streams[videoStreamIndex_]->codecpar->codec_id);
    codecCtx_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(
        codecCtx_, formatCtx_->streams[videoStreamIndex_]->codecpar);

    AVBufferRef* hwDeviceCtx = nullptr; // 🚀 올바른 위치: codecCtx_ 생성 직후 하드웨어 가속기 연결
    if (av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) >= 0) {
        fprintf(stderr, "[Video] 하드웨어 가속(D3D11VA) 초기화 성공\n");
        codecCtx_->hw_device_ctx = av_buffer_ref(hwDeviceCtx); // 🚀 소유권 이전
        av_buffer_unref(&hwDeviceCtx); // 🚀 안전하게 참조 카운트 감소
    } else {
        fprintf(stderr, "[Video] 하드웨어 가속 초기화 실패, 순수 CPU 디코딩으로 진행\n");
    }

    if (codecCtx_->width <= 0 || codecCtx_->height <= 0) {
        fprintf(stderr, "[Video] Resolution unknown - decoding as is\n");
    }
    codecCtx_->thread_count = 0;
    codecCtx_->flags |= AV_CODEC_FLAG_LOW_DELAY;
    codecCtx_->skip_loop_filter = AVDISCARD_ALL;
    codecCtx_->skip_frame = AVDISCARD_DEFAULT;

    if (avcodec_open2(codecCtx_, codec, nullptr) < 0) {
        fprintf(stderr, "[Video] 코덱 오픈 실패\n");
        cleanupFFmpeg();
        return false;
    }

    frame = av_frame_alloc();
    frameRGB = av_frame_alloc();
    packet = av_packet_alloc();

    fprintf(stderr, "[Video] 디코딩 시작\n");

    int consecutiveErrors = 0;
    constexpr int kMaxErrors = 50;

    while (!stopThread_) {
        int ret = av_read_frame(formatCtx_, packet);
        if (ret >= 0) {
            consecutiveErrors = 0;
            if (packet->stream_index == videoStreamIndex_) {
                if (avcodec_send_packet(codecCtx_, packet) == 0) {
                    while (true) {
                        int recv = avcodec_receive_frame(codecCtx_, frame);
                        if (recv != 0)
                            break;

                        AVFrame* targetFrame = frame; // 🚀 처리용 프레임 포인터 할당
                        if (frame->format == AV_PIX_FMT_D3D11 || frame->format == AV_PIX_FMT_DXVA2_VLD) {
                            if (av_hwframe_transfer_data(swFrame, frame, 0) < 0) {
                                fprintf(stderr, "[Video] HW -> CPU 프레임 전송 실패\n");
                                continue;
                            }
                            targetFrame = swFrame; // 🚀 시스템 메모리로 내려받은 프레임으로 대상 변경
                        }

                        int srcWidth = targetFrame->width;
                        int srcHeight = targetFrame->height;
                        if (srcWidth <= 0 || srcHeight <= 0)
                            continue;

                        int targetWidth = srcWidth;
                        int targetHeight = srcHeight;

                        if (!swsCtx_ || width != srcWidth || height != srcHeight ||
                            prevTargetWidth != targetWidth ||
                            prevTargetHeight != targetHeight) {
                            width = srcWidth;
                            height = srcHeight;
                            prevTargetWidth = targetWidth;
                            prevTargetHeight = targetHeight;

                            if (swsCtx_)
                                sws_freeContext(swsCtx_);

                            int numBytes = av_image_get_buffer_size(
                                AV_PIX_FMT_RGBA, targetWidth, targetHeight, 4);

                            const int poolSize = (targetWidth > 1920)
                                                     ? Config::VIDEO_BUFFER_POOL_SIZE_4K
                                                     : Config::VIDEO_BUFFER_POOL_SIZE_HD;

                            bufferPool_.clear();
                            for (int i = 0; i < poolSize; ++i) {
                                auto buf = std::make_shared<std::vector<uint8_t>>(
                                    numBytes + AV_INPUT_BUFFER_PADDING_SIZE);
                                bufferPool_.push_back(buf);
                            }

                            swsCtx_ = sws_getContext(srcWidth, srcHeight, static_cast<AVPixelFormat>(targetFrame->format), targetWidth, targetHeight, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr); // 🚀 targetFrame의 포맷으로 SwsContext 갱신
                        }

                        std::shared_ptr<std::vector<uint8_t>> targetBuffer = nullptr;
                        for (auto &buf : bufferPool_) {
                            if (buf.use_count() == 1) {
                                targetBuffer = buf;
                                break;
                            }
                        }
                        if (!targetBuffer)
                            continue;

                        av_image_fill_arrays(frameRGB->data, frameRGB->linesize, targetBuffer->data(), AV_PIX_FMT_RGBA, targetWidth, targetHeight, 4);

                        sws_scale(swsCtx_, (const uint8_t *const *)targetFrame->data, targetFrame->linesize, 0, srcHeight, frameRGB->data, frameRGB->linesize); // 🚀 targetFrame 기반으로 스케일링 수행

                        if (onFrameReady) {
                            onFrameReady(targetBuffer, targetWidth, targetHeight, frameRGB->linesize[0]);
                        }
                    }
                }
            }
            av_packet_unref(packet);
        } else {
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
    if (swFrame)
        av_frame_free(&swFrame); // 🚀 사용이 끝난 임시 프레임 메모리 정상 해제

    cleanupFFmpeg();
    return success;
}

void Video::cleanupFFmpeg() {
  if (swsCtx_) {
    sws_freeContext(swsCtx_);
    swsCtx_ = nullptr;
  }
  if (codecCtx_) {
    avcodec_free_context(&codecCtx_);
    codecCtx_ = nullptr;
  }
  if (formatCtx_) {
    avformat_close_input(&formatCtx_);
    formatCtx_ = nullptr;
  }
  videoStreamIndex_ = -1;
}
