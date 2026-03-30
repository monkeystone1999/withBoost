#pragma once
#include <chrono>
#include <string>


/**
 * @file AuthManager.hpp
 * @brief 사용자 인증 정보 및 권한 관리자
 *
 * 이 파일은 애플리케이션에 로그인한 현재 사용자의 프로필,
 * 접근 권한(Role) 및 세션 상태를 관리하는 기능을 정의합니다.
 */

/** @brief 사용자의 시스템 접근 권한 등급 */
enum class UserRole {
  User, /**< 일반 사용자: 조회 및 기본 조작 권한 */
  Admin /**< 관리자: 시스템 설정 및 제어 권한 */
};

/**
 * @struct UserData
 * @brief 개별 사용자의 상세 정보 및 통계 데이터
 */
struct UserData {
  std::string userId;                              /**< 고유 식별자 */
  std::string username;                            /**< 표시용 이름 */
  std::string email;                               /**< 연락처 정보 */
  UserRole role;                                   /**< 할당된 권한 역할 */
  bool isOnline;                                   /**< 현재 접속 여부 */
  std::chrono::system_clock::time_point lastLogin; /**< 마지막 로그인 시간 */
  std::string ipAddress;                           /**< 접속 IP 주소 */
  int activeCameras; /**< 현재 감시 중인 카메라 대수 */

  UserData() : role(UserRole::User), isOnline(false), activeCameras(0) {}
};

/**
 * @class AuthManager
 * @brief 사용자 인증 프로필 및 로그인 세션 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. Login() 메서드를 통해 인증 서버와 연동하여 세션을 활성화합니다.
 * 2. getCurrentUser()를 호출하여 현재 작업자의 권한 등급을 확인하고 UI 기능을
 * 제한/개방합니다.
 * 3. 프로그램 종료 또는 정지 시 Logout()을 통해 세션 정보를 초기화합니다.
 */
class AuthManager {
public:
  /**
   * @brief 로그인 수행 및 세션 정보 업데이트
   * @param username 사용자 계정 이름
   * @param password 평면 텍스트 비밀번호 (내부 암호화 처리 예정)
   * @note 실제 구현 시 네트워크 엔진을 통한 서버 검증 절차가 포함됩니다.
   */
  void Login(const std::string &username, const std::string &password) {
    // ... Login logic ...
  }

  /** @brief 현재 로그인된 사용자의 세션 정보 파기 */
  void Logout() { currentUser_ = UserData(); }

  /** @return const UserData& 현재 로그인된 사용자의 데이터 참조 */
  const UserData &getCurrentUser() const { return currentUser_; }

private:
  UserData currentUser_; /**< 현재 활성화된 사용자 프로필 */
};

/**
 * @section Workflow Guide
 *
 * **[AuthManager 보안 가이드]**
 *
 * 1. 권한 기반 분기:
 *    - UI 컨트롤러는 항상 `getCurrentUser().role`을 체크하여 비인가 사용자의
 * 설정 접근을 차단해야 합니다.
 *
 * 2. 상태 동기화:
 *    - `Logout()` 호출 시 `currentUser_`가 기본 생성자로 초기화되므로, UI
 * 모델에 해당 소멸 이벤트를 알려야 합니다.
 */
