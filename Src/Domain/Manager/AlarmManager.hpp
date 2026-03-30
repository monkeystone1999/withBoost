#pragma once
#include "../../Thread/ThreadEngine.hpp"
#include <deque>
#include <functional>
#include <string>


/**
 * @file AlarmManager.hpp
 * @brief 알람 이벤트 발생 및 디스패칭 관리
 *
 * 이 파일은 네트워크 페이로드(JSON)를 분석하여 알람 이벤트를 생성하고,
 * 이를 비동기적으로 상위 레이어에 전달하는 기능을 정의합니다.
 */

/**
 * @struct AlarmEvent
 * @brief 알람의 세부 정보를 담는 구조체
 */
struct AlarmEvent {
  std::string title;  /**< 알람 제목 (예: "침입 감지") */
  std::string detail; /**< 알람 상세 내용 */
  int severity = 1;   /**< 위험도 수준 (1: 일반, 2: 경고, 3: 심각) */
};

/**
 * @class AlarmManager
 * @brief JSON 원시 데이터를 알람 객체로 변환 및 배포하는 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 생성 시 비동기 처리를 위한 ThreadEngine 참조를 주입합니다.
 * 2. dispatch() 메서드에 JSON 문자열과 처리 완료 후 실행할 콜백을 전달합니다.
 * 3. 내부 스레드 풀에서 파싱 후 유효한 알람일 경우에만 콜백을 실행합니다.
 */
class AlarmManager {
public:
  /** @brief 알람 이벤트 발생 시 호출될 콜백 함수 정의 */
  using Callback = std::function<void(AlarmEvent)>;

  /**
   * @brief 알람 매니저 생성자
   * @param pool 비동기 작업 실행을 위한 스레드 풀 엔진 참조
   */
  explicit AlarmManager(ThreadEngine &pool) : pool_(pool) {}

  /**
   * @brief JSON 데이터를 분석하여 알람 이벤트 배포
   * @param json 네트워크로부터 수신된 원시 JSON 문자열
   * @param cb 유효한 알람 분석 시 호출될 콜백 함수
   * @note 이 메서드는 즉시 반환되며, 실제 분석 및 콜백은 pool_의 스레드에서
   * 실행됩니다.
   */
  void dispatch(const std::string &json, Callback cb);

private:
  ThreadEngine &pool_; /**< 주입된 스레드 풀 참조 */
};

/**
 * @section Workflow Guide
 *
 * **[AlarmManager 분석 워크플로우]**
 *
 * 1. 수신: 네트워크 엔진이 `type: alarm` 패킷을 수신하면 `dispatch`를
 * 호출합니다.
 * 2. 분석: `dispatch`는 `ThreadEngine`에 작업을 위임하여 JSON을 파싱하고 필드를
 * 추출합니다.
 * 3. 배포: 유효성이 검증된 `AlarmEvent` 객체를 생성하여 등록된 `cb`로
 * 전달합니다.
 */
