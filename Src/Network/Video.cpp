#include "Video.hpp"
#include "../Config.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

static void ffmpeg_log_callback(void *, int level, const char *fmt,
                                va_list vl) {
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
  if (decodeThread_.joinable())
    decodeThread_.join();
  cleanupFFmpeg();
}

void Video::decodeLoopFFmpeg(const std::string &url) {
  while (!stopThread_) {
    bool ok = tryOnceFFmpeg(url);
    if (stopThread_)
      break;
    if (!ok) {
      fprintf(stderr, "[Video] Stream Failed\n");
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
  }
}

bool Video::tryOnceFFmpeg(const std::string &url) {
  AVFrame *frame = av_frame_alloc();
  AVFrame *swFrame = av_frame_alloc();
  AVPacket *packet = av_packet_alloc();
  AVDictionary *options = nullptr;
  const AVCodec *codec = nullptr;
  int width = 0, height = 0;
  bool success = true;

  av_log_set_callback(ffmpeg_log_callback);
  avformat_network_init();

  formatCtx_ = avformat_alloc_context();
  av_dict_set(&options, "rtsp_transport", "udp", 0);
  av_dict_set(&options, "stimeout", "5000000", 0);

  if (avformat_open_input(&formatCtx_, url.c_str(), nullptr, &options) != 0) {
    av_frame_free(&frame);
    av_frame_free(&swFrame);
    av_packet_free(&packet);
    if (options)
      av_dict_free(&options);
    return false;
  }

  if (avformat_find_stream_info(formatCtx_, nullptr) < 0) {
    cleanupFFmpeg();
    return false;
  }

  videoStreamIndex_ = -1;
  for (unsigned i = 0; i < formatCtx_->nb_streams; ++i) {
    if (formatCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStreamIndex_ = i;
      break;
    }
  }
  if (videoStreamIndex_ == -1) {
    cleanupFFmpeg();
    return false;
  }

  codec = avcodec_find_decoder(
      formatCtx_->streams[videoStreamIndex_]->codecpar->codec_id);
  codecCtx_ = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(
      codecCtx_, formatCtx_->streams[videoStreamIndex_]->codecpar);

  AVBufferRef *hwDeviceCtx = nullptr;
  if (av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_D3D11VA, nullptr,
                             nullptr, 0) >= 0) {
    codecCtx_->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
    av_buffer_unref(&hwDeviceCtx);
  }

  codecCtx_->thread_count = 0;
  if (avcodec_open2(codecCtx_, codec, nullptr) < 0) {
    cleanupFFmpeg();
    return false;
  }

  while (!stopThread_) {
    if (av_read_frame(formatCtx_, packet) < 0)
      break;
    if (packet->stream_index == videoStreamIndex_) {
      if (avcodec_send_packet(codecCtx_, packet) == 0) {
        while (avcodec_receive_frame(codecCtx_, frame) == 0) {
          AVFrame *targetFrame = frame;
          if (frame->format == AV_PIX_FMT_D3D11) {
            if (av_hwframe_transfer_data(swFrame, frame, 0) < 0)
              continue;
            targetFrame = swFrame;
          }

          int srcW = targetFrame->width, srcH = targetFrame->height;
          if (srcW <= 0 || srcH <= 0)
            continue;

          if (targetFrame->format == AV_PIX_FMT_NV12 ||
              targetFrame->format == AV_PIX_FMT_NV21) {

            int ySize = targetFrame->linesize[0] * srcH;
            int uvSize = targetFrame->linesize[1] * (srcH / 2);
            int requiredBytes = ySize + uvSize;

            if (width != srcW || height != srcH) {
              width = srcW;
              height = srcH;
              bufferPool_.clear();
              int poolSize = (width > 2000) ? Config::VIDEO_BUFFER_POOL_SIZE_4K
                                            : Config::VIDEO_BUFFER_POOL_SIZE_HD;
              for (int i = 0; i < poolSize; ++i)
                bufferPool_.push_back(
                    std::make_shared<std::vector<uint8_t>>(requiredBytes + 64));
            }

            std::shared_ptr<std::vector<uint8_t>> targetBuf = nullptr;
            for (auto &b : bufferPool_)
              if (b.use_count() == 1) {
                targetBuf = b;
                break;
              }
            if (!targetBuf)
              continue;

            std::memcpy(targetBuf->data(), targetFrame->data[0], ySize);
            std::memcpy(targetBuf->data() + ySize, targetFrame->data[1],
                        uvSize);

            if (onFrameReady) {
              onFrameReady({targetBuf, srcW, srcH, targetFrame->linesize[0],
                            targetFrame->linesize[1], ySize, 0});
            }
          }
        }
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

void Video::cleanupFFmpeg() {
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
