#include "AlarmService.hpp"
#include "../../Src/Domain/AlarmManager.hpp"

AlarmService::AlarmService(AlarmManager *dispatcher, QObject *parent)
    : QObject(parent), dispatcher_(dispatcher) {

  // 도메인 매니저에 콜백 등록 (패킷 수신 시마다 실행)
  if (dispatcher_) {
    // Note: Core 혹은 Bridge 레이어에서 실제 dispatch 루프를 연결합니다.
  }
}

AlarmService::~AlarmService() {}

void AlarmService::onAlarm(AlarmEvent ev) {
  emit alarmTriggered(QString::fromStdString(ev.title),
                      QString::fromStdString(ev.detail), ev.severity);
}

/**
 * @section Workflow Guide
 *
 * **[AlarmService 내부 동작 메커니즘]**
 *
 * - 레이어 분리: 본 클래스는 순수 C++ 도메인 데이터(`AlarmEvent`)와 Qt UI
 * 레이어 사이의 타입 변환기(Type Converter) 역할을 수행합니다.
 * - 스레드 안전성: `QString::fromStdString` 등 UI 관련 작업은 오직 GUI
 * 스레드에서만 수행되도록 설계되어 있습니다.
 */
