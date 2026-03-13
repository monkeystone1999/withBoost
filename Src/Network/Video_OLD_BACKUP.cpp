#include "Video.hpp"
#include "../Config.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <libyuv.h> // 🚀 [추가] 구글 libyuv 초고속 변환 라이브러리


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

struct ColorParams {
  bool swapUV = false;
  bool fullRange = false;
  bool use709 = true;
};

static inline float ClipPenalty(float v) {
  if (v < 0.0f)
    return -v;
  if (v > 1.0f)
    return v - 1.0f;
  return 0.0f;
}

static float ScoreYuvParams(const uint8_t *buf, int w, int h, int strideY,
                            int strideUV, int uvOffset, const ColorParams &p) {
  if (!buf || w <= 0 || h <= 0 || strideY <= 0 || strideUV <= 0 ||
      uvOffset <= 0)
    return 1e9f;

  const int grid = 8;
  const int stepX = std::max(1, w / grid);
  const int stepY = std::max(1, h / grid);
  float score = 0.0f;

  for (int y = 0; y < h; y += stepY) {
    const uint8_t *rowY = buf + y * strideY;
    const uint8_t *rowUV = buf + uvOffset + (y / 2) * strideUV;
    for (int x = 0; x < w; x += stepX) {
      const int xi = std::min(x, w - 1);
      const uint8_t Y = rowY[xi];
      const int uvIndex = (xi / 2) * 2;
      uint8_t U = rowUV[uvIndex + 0];
      uint8_t V = rowUV[uvIndex + 1];
      if (p.swapUV)
        std::swap(U, V);

      float yf = Y / 255.0f;
      float uf = U / 255.0f - 0.5f;
      float vf = V / 255.0f - 0.5f;

      if (!p.fullRange)
        yf = 1.1643f * (yf - 0.0625f);

      float r, g, b;
      if (p.use709) {
        r = yf + 1.7927f * vf;
        g = yf - 0.2132f * uf - 0.5329f * vf;
        b = yf + 2.1124f * uf;
      } else {
        r = yf + 1.5960f * vf;
        g = yf - 0.3918f * uf - 0.8130f * vf;
        b = yf + 2.0172f * uf;
      }

      score += ClipPenalty(r) + ClipPenalty(g) + ClipPenalty(b) * 2.0f;
      score += std::max(0.0f, g - (r + b) * 0.5f); // Green bias penalty
    }
  }
  return score;
}

static ColorParams ChooseBestParams(const uint8_t *buf, int w, int h,
                                    int strideY, int strideUV, int uvOffset,
                                    const ColorParams &meta) {
  ColorParams best = meta;
  float bestScore =
      ScoreYuvParams(buf, w, h, strideY, strideUV, uvOffset, meta);

  for (bool sw : {false, true}) {
    for (bool fr : {false, true}) {
      for (bool m709 : {false, true}) {
        ColorParams c = {sw, fr, m709};
        float s = ScoreYuvParams(buf, w, h, strideY, strideUV, uvOffset, c);
        if (s < bestScore) {
          bestScore = s;
          best = c;
        }
      }
    }
  }
  return best;
}

namespace {
bool ResolveFullRange(const AVFrame *frame) {
  return (frame->color_range == AVCOL_RANGE_JPEG);
}
bool ResolveUse709(const AVFrame *frame) {
  if (frame->colorspace == AVCOL_SPC_BT709)
    return true;
  if (frame->colorspace == AVCOL_SPC_BT470BG ||
      frame->colorspace == AVCOL_SPC_SMPTE170M)
    return false;
  return frame->height > 576; // Fallback: HD -> 709, SD -> 601
}
} // namespace

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
      fprintf(stderr, "[Video] 연결 실패 — 3초 후 재시도: %s\n", url.c_str());
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

  // 🚀 하드웨어 가속 시도 (D3D11VA)
  AVBufferRef *hwDeviceCtx = nullptr;
  if (av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_D3D11VA, nullptr,
                             nullptr, 0) >= 0) {
    codecCtx_->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
    av_buffer_unref(&hwDeviceCtx);
  }

  codecCtx_->thread_count = 0; // 최적화된 스레드 수 자동 선택
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

          // 버퍼 풀 초기화 및 해상도 변경 대응
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

          if (targetFrame->format == AV_PIX_FMT_NV12 ||
              targetFrame->format == AV_PIX_FMT_NV21) {
            std::memcpy(targetBuf->data(), targetFrame->data[0], ySize);
            std::memcpy(targetBuf->data() + ySize, targetFrame->data[1],
                        uvSize);

            if (onFrameReady) {
              ColorParams meta = {(targetFrame->format == AV_PIX_FMT_NV21),
                                  ResolveFullRange(targetFrame),
                                  ResolveUse709(targetFrame)};
              ColorParams chosen = ChooseBestParams(
                  targetBuf->data(), srcW, srcH, targetFrame->linesize[0],
                  targetFrame->linesize[1], ySize, meta);

              onFrameReady({targetBuf, srcW, srcH, targetFrame->linesize[0],
                            targetFrame->linesize[1], ySize, PixelFormat::NV12,
                            chosen.swapUV, chosen.fullRange, chosen.use709});
            }
          } else {
            // 기타 포맷은 RGBA로 변환 (libyuv 사용)
            auto rgbaBuf =
                std::make_shared<std::vector<uint8_t>>(srcW * srcH * 4 + 64);
            if (libyuv::I420ToABGR(
                    targetFrame->data[0], targetFrame->linesize[0],
                    targetFrame->data[1], targetFrame->linesize[1],
                    targetFrame->data[2], targetFrame->linesize[2],
                    rgbaBuf->data(), srcW * 4, srcW, srcH) == 0) {
              if (onFrameReady)
                onFrameReady({rgbaBuf, srcW, srcH, srcW * 4, 0, 0,
                              PixelFormat::RGBA, false, false, true});
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
