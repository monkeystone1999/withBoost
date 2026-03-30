#pragma once
#include "AlarmManager.hpp"
#include <deque>

/**
 * @file AlarmBridge.hpp
 * @brief 알람 데이터와 UI/네트워크 간의 중계자
 *
 * 이 파일은 수신된 알람 이벤트들을 이력(History) 형태로 관리하는 기능을
 * 제공합니다. 네트워크 레이어에서 발생한 실시간 알람을 UI 레이어가 참조할 수
 * 있도록 버퍼링합니다.
 */

/**
 * @class AlarmBridge
 * @brief 알람 이벤트 이력 관리 및 중계 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 외부 소스(AlarmManager 등)로부터 발생한 이벤트를 AddEvent()로
 * 전달받습니다.
 * 2. 내부 큐에 이벤트를 저장하며, 설정된 최대 개수(100개)를 초과하면 오래된
 * 순으로 삭제합니다.
 * 3. UI 모델은 이 클래스를 참조하여 알람 리스트 뷰를 갱신합니다.
 */
class AlarmBridge {
public:
  /**
   * @brief 이벤트 이력에 새 알람 추가
   * @param ev 추가할 알람 이벤트 객체
   * @note 스레드 안전성은 보장되지 않으므로 호출 측에서의 동기화가 필요할 수
   * 있습니다.
   */
  void AddEvent(AlarmEvent ev) {
    events_.push_back(std::move(ev));
    if (events_.size() > 100)
      events_.pop_front();
  }

private:
  std::deque<AlarmEvent> events_; /**< 알람 이벤트 저장용 데크(Deque) */
};

/**
 * @section Workflow Guide
 *
 * **[AlarmBridge 통합 워크플로우]**
 *
 * 1. 초기화: 시스템 기동 시 전역 또는 세션별 `AlarmBridge` 인스턴스를
 * 생성합니다.
 * 2. 수집: `AlarmManager`의 콜백 함수 내에서 `AlarmBridge::AddEvent`를
 * 호출하도록 구성합니다.
 * 3. 소비: UI 리스트 모델은 `events_`의 변경 사항을 감지하여 화면에 최신 알람을
 * 출력합니다.
 */
