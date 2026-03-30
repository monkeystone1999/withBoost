#include "NetworkManager.hpp"
#include "../Crypt/DtlsEngine.hpp"
#include "../Crypt/TlsEngine.hpp"
#include "NetworkEngine.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

NetworkManager::NetworkManager() {
  /// TLS/DTLS 컨텍스트 초기화
  DTLS::InitCookie();
  tlsCtx_ = TLS::ClientContext();
  dtlsCtx_ = DTLS::ClientContext();
  mediaDtlsCtx_ = MediaDTLS::ClientContext();
}

NetworkManager::~NetworkManager() {
  if (tlsCtx_)
    SSL_CTX_free(tlsCtx_);
  if (dtlsCtx_)
    SSL_CTX_free(dtlsCtx_);
  if (mediaDtlsCtx_)
    SSL_CTX_free(mediaDtlsCtx_);
}

void NetworkManager::Packets(MessageType Type, Payload Body) {
  try {
    auto body_ = nlohmann::json::parse(Body->begin(), Body->end());

    switch (Type) {
    case MessageType::CAMERA:
      CameraBridge_(body_.value("ip", ""), body_.value("url", ""));
      break;
    case MessageType::META:
      CameraBridge_(
          body_.value("ip", ""),
          std::make_pair(std::array<float, 4>{body_.value("tmp", 0.0f),
                                              body_.value("tilt", 0.0f),
                                              body_.value("light", 0.0f),
                                              body_.value("hum", 0.0f)},
                         body_.value("dir", "")));
      break;
    case MessageType::AI:
      CameraBridge_(body_.value("ip", ""),
                    std::vector<uint8_t>(Body->begin(), Body->end()));
      break;
    case MessageType::IMAGE:
      CameraBridge_(body_.value("ip", ""),
                    ImageMeta{body_.value("total_frames", 0),
                              body_.value("jpeg_size", 0),
                              body_.value("frame_index", 0),
                              body_.value("timestamp_ms", int64_t(0))});
      // Pass binary image payload (JPEG) to the UI bridge
      CameraBridge_(body_.value("ip", ""),
                    std::vector<uint8_t>(Body->begin(), Body->end()));
      break;
    case MessageType::SUCCESS:
      AuthBridge_(body_.value("username", ""),
                  body_.value("state", body_.value("message", "")));
      break;
    case MessageType::FAIL:
      AuthBridge_("", body_.value("error", "unknown error"));
      break;
    case MessageType::ASSIGN:
      AuthBridge_("", body_.dump());
      break;
    case MessageType::AVAILABLE:
    case MessageType::DEVICE:
      StatusBridge_(Body);
      break;
    default:
      break;
    }
  } catch (...) {
    /// JSON 파싱 실패 무시
  }
}

/**
 * @section Workflow Guide
 *
 * **[NetworkManager 패킷 처리 상세]**
 *
 * 1. JSON 기반 페이로드 처리:
 *    - 모든 수신 패킷 바디는 표준 JSON 형식을 따를 것으로 기대됩니다.
 *    - `nlohmann::json::parse`가 실패할 경우 예외를 캐치하여 시스템 중단을
 * 방지합니다.
 *
 * 2. 메시지 타입별 시그널링:
 *    - `MessageType::CAMERA`: 신규 카메라 발견 또는 기본 정보 업데이트.
 *    - `MessageType::META`: 온도, 틸트, 조도, 습도 등 센서 데이터 전송.
 *    - `MessageType::AI`: 객체 탐지 등 로그 데이터 전달.
 *    - `MessageType::IMAGE`: 썸네일 또는 정지 영상 스트림 정보 전달.
 *    - `MessageType::SUCCESS / FAIL`: 로그인 및 권한 인증 결과.
 *
 * 3. 확장 가이드:
 *    - 새로운 데이터 타입을 추가할 경우 `NetworkProtocol.hpp`에 `MessageType`을
 * 정의하고, 본 파일의 `Packets` 스위치 문에 해당 로직을 추가한 뒤 신규 브릿지
 * 시그널을 통해 UI에 알리십시오.
 */

void NetworkManager::startTextConnect() {
  if (textConnect_) {
    textConnect_->connect();
    textConnect_->run();
    textConnect_->Read();
  }
}

void NetworkManager::sendTextMessage(MessageType type,
                                     const std::string &jsonBody) {
  if (textConnect_) {
    textConnect_->Send(type, jsonBody);
  }
}
