#include "AlarmManager.hpp"
#include <nlohmann/json.hpp>

void AlarmManager::dispatch(const std::string &json, Callback cb) {
  if (!cb)
    return;
  pool_.Submit([json, cb = std::move(cb)]() {
    auto parsed = nlohmann::json::parse(json, nullptr, false);
    if (!parsed.is_object() || parsed.value("type", "") != "alarm")
      return;

    AlarmEvent ev;
    ev.title = parsed.value("title", "AI Alert");
    ev.detail = parsed.value("detail", "Anomaly detected");
    ev.severity = parsed.value("severity", 1);
    cb(ev);
  });
}

/**
 * @section Workflow Guide
 *
 * **[AlarmManager 구현 및 확장 가이드]**
 *
 * 1. 비동기 처리:
 *    - `nlohmann::json::parse` 및 객체 생성 과정이 스레드 풀에서 작업(Task)
 * 단위로 실행됩니다.
 *    - 콜백 함수(`cb`) 내부에서 UI를 직접 조작할 경우 반드시 메인 스레드로의
 * 마샬링이 필요합니다.
 *
 * 2. 패킷 필터링 로직:
 *    - JSON 루트에 `type` 필드가 `"alarm"`인 경우에만 알람으로 간주합니다.
 *    - 파싱 실패 시 예외를 던지지 않고 조용히 리턴하도록 설계되어 안정성을
 * 확보하였습니다.
 *
 * 3. 데이터 매핑 정보:
 *    - `title` -> `title` (기본값: "AI Alert")
 *    - `detail` -> `detail` (기본값: "Anomaly detected")
 *    - `severity` -> `severity` (기본값: 1)
 */
