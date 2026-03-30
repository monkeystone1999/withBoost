#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <string>


/**
 * @file StatusManager.hpp
 * @brief 서버 상태 및 사용자 목록 관리자
 *
 * 이 파일은 서버의 물리적 자원(온도, 메모리, CPU) 사용량 이력과
 * 현재 시스템에 로그인하여 활동 중인 사용자들의 메타데이터를 관리합니다.
 */

/** @brief 사용자의 시스템 권한 등급 상태 */
enum class UserState : uint8_t {
  Admin = 0, /**< 관리자: 모든 제어 권한 보유 */
  Normal,    /**< 일반 사용자: 표준 운용 권한 */
  Pending    /**< 승인 대기: 제한적 접근 권한 */
};

/**
 * @struct ServerStatus
 * @brief 서버 자원 모니터링 데이터 저장 구조체
 */
struct ServerStatus {
  std::deque<float> temp{20, 0.0f}, memory{20, 0.0f},
      cpu{20, 0.0f}; /**< 최근 20개 지점의 [온도, 메모리, CPU] 시계열 데이터 */

  /**
   * @brief 자원 사용량 데이터 추가
   * @param meta [0:온도, 1:메모리, 2:CPU] 데이터 배열
   */
  void Add(const std::array<float, 3> &meta);
};

/**
 * @struct UserMeta
 * @brief 개별 사용자의 요약 정보
 */
struct UserMeta {
  std::string name_; /**< 사용자 계정 또는 이름 */
  UserState state_;  /**< 현재 할당된 권한 상태 */

};

/**
 * @class StatusManager
 * @brief 시스템 전역 상태 및 세션 사용자 중앙 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. getStatus()를 호출하여 서버 자원 데이터 객체를 획득하고 시각화 데이터로
 * 활용합니다.
 * 2. AddUser()를 통해 신규 로그인 사용자를 도메인 모델에 등록합니다.
 * 3. getUsers()를 통해 현재 접속된 전체 사용자 명단을 맵(Map) 형태로
 * 조회합니다.
 */
class StatusManager {
public:
  /**
   * @brief 신규 사용자 등록
   * @param name 사용자 이름
   * @param state 권한 문자열 ("Admin", "Normal" 등)
   */
  void AddUser(std::string name, std::string state);

  /** @return ServerStatus& 서버 자원 상태 객체 참조 */
  ServerStatus &getStatus() { return status_; }

  /** @return std::map<uint8_t, UserMeta>& 등록된 전체 사용자 맵 참조 (Key:
   * UserId) */
  std::map<uint8_t, UserMeta> &getUsers() { return users_; }

private:
  ServerStatus status_;               /**< 서버 하드웨어 상태 모델 */
  uint8_t nextUserId_ = 0;            /**< 신규 유저에게 할당할 자동 증가 ID */
  std::map<uint8_t, UserMeta> users_; /**< ID 기반 사용자 저장소 */
  std::map<std::string, UserState>
      nameToState_; /**< 사용자 이름 기반 중복 체크 및 상태 매핑 테이블 */
};

/**
 * @section Workflow Guide
 *
 * **[StatusManager 모니터링 워크플로우]**
 *
 * 1. 자원 업데이트 flow:
 *    - 주기적으로 수신되는 서버 상태 패킷을 `status_.Add()`를 통해 누적합니다.
 *    - 시계열 데이터(Deque) 형식으로 관리되어 UI 차트 등에서 즉시 사용
 * 가능합니다.
 *
 * 2. 세션 동기화:
 *    - `AuthBridge` 등을 통해 전달된 인증 결과가 `AddUser`를 거쳐 `users_` 맵에
 * 반영됩니다.
 *    - UI 레이어는 이 맵을 순회하며 접속자 명단 UI를 실시간으로 갱신합니다.
 */
