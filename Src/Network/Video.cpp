#include "Video.hpp"
#include "../Config.hpp"
#include <chrono>
#include <cstdio>
#include <libyuv.h> // 🚀 [추가] 구글 libyuv 초고속 변환 라이브러리

static void ffmpeg_log_callback(void *, int level, const char *fmt, va_list vl) {
    if (level > AV_LOG_WARNING)
        return;
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, vl);
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
    while (!stopThread_) {
        bool ok = tryOnceFFmpeg(url);
        if (stopThread_)
            break;
        if (!ok) {
            fprintf(stderr, "[Video] 연결 실패/끊김 — 3초 후 재시도: %s\n", url.c_str());
            for (int i = 0; i < 30 && !stopThread_; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    fprintf(stderr, "[Video] 디코딩 루프 종료\n");
}

bool Video::tryOnceFFmpeg(const std::string &url) {
    AVFrame *frame = nullptr;
    AVFrame *swFrame = av_frame_alloc(); // 🚀 하드웨어 -> 시스템 메모리 다운로드용 임시 프레임
    // 🚀 [삭제] libyuv는 직접 벡터에 쓰기 때문에 frameRGB가 필요 없습니다.
    AVPacket *packet = nullptr;
    AVDictionary *options = nullptr;
    const AVCodec *codec = nullptr;
    int width = 0, height = 0;
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

    AVBufferRef* hwDeviceCtx = nullptr;
    if (av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) >= 0) {
        fprintf(stderr, "[Video] 하드웨어 가속(D3D11VA) 초기화 성공\n");
        codecCtx_->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
        av_buffer_unref(&hwDeviceCtx);
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

                        AVFrame* targetFrame = frame;
                        if (frame->format == AV_PIX_FMT_D3D11 || frame->format == AV_PIX_FMT_DXVA2_VLD) {
                            if (av_hwframe_transfer_data(swFrame, frame, 0) < 0) {
                                fprintf(stderr, "[Video] HW -> CPU 프레임 전송 실패\n");
                                continue;
                            }
                            targetFrame = swFrame;
                        }

                        int srcWidth = targetFrame->width;
                        int srcHeight = targetFrame->height;
                        if (srcWidth <= 0 || srcHeight <= 0)
                            continue;

                        int targetWidth = srcWidth;
                        int targetHeight = srcHeight;
                        int numBytes = targetWidth * targetHeight * 4;

                        const int poolSize = (targetWidth > 1920)
                                                 ? Config::VIDEO_BUFFER_POOL_SIZE_4K
                                                 : Config::VIDEO_BUFFER_POOL_SIZE_HD;

                        // 해상도가 변경되었을 때만 버퍼 풀 초기화
                        if (width != srcWidth || height != srcHeight) {
                            width = srcWidth;
                            height = srcHeight;

                            bufferPool_.clear();
                            for (int i = 0; i < poolSize; ++i) {
                                auto buf = std::make_shared<std::vector<uint8_t>>(
                                    numBytes + AV_INPUT_BUFFER_PADDING_SIZE);
                                bufferPool_.push_back(buf);
                            }
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

                        // =====================================================================
                        // 🚀 [추가/수정] libyuv 초고속 색상 변환 (sws_scale 대체)
                        // =====================================================================
                        int ret_yuv = -1;

                        // 1. 하드웨어 가속(D3D11)을 거쳐 내려온 프레임은 일반적으로 NV12 포맷입니다.
                        if (targetFrame->format == AV_PIX_FMT_NV12) {
                            // libyuv의 ABGR 계열 함수는 메모리에 [R, G, B, A] 순서로 출력해 줍니다. (Qt 호환성)
                            ret_yuv = libyuv::NV12ToABGR(
                                targetFrame->data[0], targetFrame->linesize[0], // Y
                                targetFrame->data[1], targetFrame->linesize[1], // UV
                                targetBuffer->data(), targetWidth * 4,          // 출력 버퍼 및 Stride
                                targetWidth, targetHeight
                                );
                        }
                        // 2. 순수 CPU 소프트웨어 디코딩을 거친 프레임은 일반적으로 YUV420P(I420) 포맷입니다.
                        else if (targetFrame->format == AV_PIX_FMT_YUV420P || targetFrame->format == AV_PIX_FMT_YUVJ420P) {
                            ret_yuv = libyuv::I420ToABGR(
                                targetFrame->data[0], targetFrame->linesize[0], // Y
                                targetFrame->data[1], targetFrame->linesize[1], // U
                                targetFrame->data[2], targetFrame->linesize[2], // V
                                targetBuffer->data(), targetWidth * 4,
                                targetWidth, targetHeight
                                );
                        } else {
                            fprintf(stderr, "[Video] 경고: libyuv 지원 범위를 벗어난 포맷 (%d) - 프레임 스킵\n", targetFrame->format);
                        }

                        if (onFrameReady && ret_yuv == 0) {
                            // 성공적으로 변환되었을 경우 QML 렌더러로 데이터 포인터 전송
                            onFrameReady(targetBuffer, targetWidth, targetHeight, targetWidth * 4);
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
    if (frame)
        av_frame_free(&frame);
    if (swFrame)
        av_frame_free(&swFrame);

    cleanupFFmpeg();
    return success;
}

void Video::cleanupFFmpeg() {
    // 🚀 [삭제] swsCtx_ 관련 해제 코드 완전히 삭제
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
