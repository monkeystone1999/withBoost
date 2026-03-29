#include "VideoEngine.hpp"
#include <chrono>
#include <cstring>
#include <thread>

VideoEngine::~VideoEngine() { stopStream(); }

int VideoEngine::interrupt_cb(void *ctx) {
    auto *v = static_cast<VideoEngine *>(ctx);
    return v->isStopping() ? 1 : 0;
}

void VideoEngine::startStream(const std::string &url, int fpsLimit) {
    if (url.empty()) return;
    stopStream();
    stopThread_ = false;

    if (fpsLimit > 0) {
        fpsLimitMs_.store(1000 / fpsLimit, std::memory_order_relaxed);
    } else {
        fpsLimitMs_.store(33, std::memory_order_relaxed);
    }

    decodeThread_ = std::thread([this, url]() { decodeLoopFFmpeg(url); });
}

void VideoEngine::setFpsLimit(int fps) {
    fpsLimitMs_.store(fps > 0 ? 1000 / fps : 33, std::memory_order_relaxed);
}

void VideoEngine::stopStream() {
    stopThread_ = true;
    if (decodeThread_.joinable()) decodeThread_.join();
    cleanupFFmpeg();
}

void VideoEngine::decodeLoopFFmpeg(const std::string &url) {
    while (!stopThread_) {
        bool ok = tryOnceFFmpeg(url);
        if (stopThread_) break;
        if (!ok) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
}

bool VideoEngine::tryOnceFFmpeg(const std::string &url) {
    AVFrame *frame = av_frame_alloc();
    AVFrame *swFrame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();
    AVDictionary *options = nullptr;
    const AVCodec *codec = nullptr;
    bool success = true;

    av_log_set_level(AV_LOG_QUIET);
    avformat_network_init();

    formatCtx_ = avformat_alloc_context();
    formatCtx_->interrupt_callback.callback = interrupt_cb;
    formatCtx_->interrupt_callback.opaque = this;

    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    av_dict_set(&options, "stimeout", "3000000", 0);
    av_dict_set(&options, "analyzeduration", "500000", 0);
    av_dict_set(&options, "probesize", "500000", 0);
    av_dict_set(&options, "fflags", "nobuffer+discardcorrupt", 0);
    av_dict_set(&options, "max_delay", "0", 0);

    auto cleanupAndFail = [&]() {
        av_frame_free(&frame);
        av_frame_free(&swFrame);
        av_packet_free(&packet);
        if (options) av_dict_free(&options);
        cleanupFFmpeg();
        return false;
    };

    if (avformat_open_input(&formatCtx_, url.c_str(), nullptr, &options) != 0) {
        return cleanupAndFail();
    }
    if (options) av_dict_free(&options);

    if (avformat_find_stream_info(formatCtx_, nullptr) < 0) {
        return cleanupAndFail();
    }

    videoStreamIndex_ = -1;
    for (unsigned i = 0; i < formatCtx_->nb_streams; ++i) {
        if (formatCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
            break;
        }
    }
    if (videoStreamIndex_ == -1) return cleanupAndFail();

    codec = avcodec_find_decoder(formatCtx_->streams[videoStreamIndex_]->codecpar->codec_id);
    codecCtx_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx_, formatCtx_->streams[videoStreamIndex_]->codecpar);

    AVBufferRef *hwDeviceCtx = nullptr;
    if (av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) >= 0) {
        codecCtx_->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
        av_buffer_unref(&hwDeviceCtx);
    }
    codecCtx_->error_concealment = 3;
    codecCtx_->thread_count = 0;

    if (avcodec_open2(codecCtx_, codec, nullptr) < 0) return cleanupAndFail();

    while (!stopThread_) {
        if (av_read_frame(formatCtx_, packet) < 0) break;
        if (packet->stream_index == videoStreamIndex_) {
            avcodec_send_packet(codecCtx_, packet);
            while (avcodec_receive_frame(codecCtx_, frame) == 0) {
                AVFrame *targetFrame = frame;
                if (frame->format == AV_PIX_FMT_D3D11) {
                    if (av_hwframe_transfer_data(swFrame, frame, 0) < 0) {
                        av_frame_unref(frame);
                        continue;
                    }
                    targetFrame = swFrame;
                }
                int srcW = targetFrame->width, srcH = targetFrame->height;
                if (srcW <= 0 || srcH <= 0) {
                    av_frame_unref(frame);
                    continue;
                }

                auto now = std::chrono::steady_clock::now();
                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime_).count();
                int limitMs = fpsLimitMs_.load(std::memory_order_relaxed);
                if (elapsedMs < limitMs) {
                    av_frame_unref(frame);
                    while (avcodec_receive_frame(codecCtx_, frame) == 0) av_frame_unref(frame);
                    if (limitMs - elapsedMs > 0)
                        std::this_thread::sleep_for(std::chrono::milliseconds(limitMs - elapsedMs));
                    break;
                }
                lastFrameTime_ = now;

                if (targetFrame->format == AV_PIX_FMT_NV12 || targetFrame->format == AV_PIX_FMT_NV21) {
                    if (onFrameReady) {
                        onFrameReady({targetFrame->data[0], targetFrame->data[1], srcW, srcH,
                                      targetFrame->linesize[0], targetFrame->linesize[1]});
                    }
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_frame_free(&swFrame);
    av_packet_free(&packet);
    cleanupFFmpeg();
    return success;
}

void VideoEngine::cleanupFFmpeg() {
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
