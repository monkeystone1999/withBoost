#include "AuthBridge.hpp"

#include "../Network/NetworkProtocol.hpp"

AuthBridge::AuthBridge(NetworkManager &networkManager,
                       StatusManager &statusManager)
    : NetworkManager_(networkManager), StatusManager_(statusManager) {
  NetworkManager_.AuthBridge_.connect(
      [this](std::string username, AuthBridgePayload payload) {
        if (std::holds_alternative<std::string>(payload)) {
          std::string state = std::get<std::string>(payload);
          StatusManager_.AddUser(username, state);

          if (username.empty() && (state.find("error") != std::string::npos ||
                                   state.find("failed") != std::string::npos)) {
            if (onLoginFailed)
              onLoginFailed(state);
          } else {
            if (onLoginSuccess)
              onLoginSuccess(state, username);
          }
        }
      });
}

void AuthBridge::login(const std::string &id, const std::string &pw) {
  NetworkManager_.startTextConnect();
  NetworkManager_.sendTextMessage(
      MessageType::LOGIN, nlohmann::json{{"id", id}, {"pw", pw}}.dump());
}

void AuthBridge::signup(const std::string &id, const std::string &email,
                        const std::string &pw) {
  NetworkManager_.startTextConnect();
  NetworkManager_.sendTextMessage(
      MessageType::ASSIGN,
      nlohmann::json{{"id", id}, {"email", email}, {"pw", pw}}.dump());
}

/**
 * @section Workflow Guide
 *
 * **[AuthBridge 구현 세부 사항]**
 *
 * 1. 시그널 처리 로직:
 *    - `std::holds_alternative<std::string>`을 통해 페이로드가 유효한 상태
 * 문자열인지 검사합니다.
 *    - 현재 구현은 단순 문자열 전달에 집중되어 있으며, 향후 정교한 권한 체크
 * 로직 추가가 용이하도록 브릿지 패턴을 적용하였습니다.
 *
 * 2. 도메인 전파:
 *    - 인증 성공 시 `StatusManager`를 통해 UI와 동기화된 메모리 내 사용자
 * 리스트를 유지합니다.
 */
