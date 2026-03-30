#pragma once
#include "../Network/NetworkManager.hpp"
#include "StatusManager.hpp"
#include <memory>

/**
 * @file AuthBridge.hpp
 * @brief 인증 및 사용자 권한 서비스 중계자
 *
 * 이 파일은 네트워크 레이어와 도메인 상태 레이어를 연결하는 브릿지 클래스를
 * 정의합니다. `NetworkManager`로부터 수신된 인증 관련 신호를 처리하여 시스템의
 * 전체 상태(`StatusManager`)에 반영합니다.
 */

/**
 * @class AuthBridge
 * @brief 네트워크 인증 시그널과 도메인 상태 간의 매핑 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 생성 시 의존성 주입(DI)을 통해 `NetworkManager`와 `StatusManager`를
 * 참조합니다.
 * 2. `NetworkManager`의 인증 시그널(`AuthBridge_`)이 발생할 때까지 대기합니다.
 * 3. 신호 수신 시 페이로드를 해석하여 `StatusManager`에 사용자 정보를
 * 추가하거나 갱신합니다.
 */
class AuthBridge {
public:
  /**
   * @brief 인증 브릿지 생성자
   * @param networkManager 인증 시그널을 제공할 네트워크 관리 객체
   * @param statusManager 인증 결과를 반영할 도메인 상태 관리 객체
   */
  AuthBridge(NetworkManager &networkManager, StatusManager &statusManager);

  void login(const std::string &id, const std::string &pw);
  void signup(const std::string &id, const std::string &email,
              const std::string &pw);

  // Callbacks for UI/Controller notification
  std::function<void(std::string, std::string)> onLoginSuccess;
  std::function<void(std::string)> onLoginFailed;
  std::function<void(std::string)> onSignupSuccess;
  std::function<void(std::string)> onSignupFailed;

private:
  NetworkManager &NetworkManager_; /**< 네트워크 통신 엔진 참조 */
  StatusManager &StatusManager_;   /**< 시스템 전역 상태 관리자 참조 */
};

/**
 * @section Workflow Guide
 *
 * **[AuthBridge 연동 워크플로우]**
 *
 * 1. 바인딩: 주입된 `NetworkManager_`의 `AuthBridge_` 시그널에 람다 핸들러를
 * 연결합니다.
 * 2. 해석: 수신된 `AuthPayload`가 문자열인지 확인하고, 사용자 이름과 상태값을
 * 추출합니다.
 * 3. 갱신: 최종적으로 `StatusManager_::AddUser`를 호출하여 도메인 모델에 로그인
 * 정보를 전파합니다.
 */
