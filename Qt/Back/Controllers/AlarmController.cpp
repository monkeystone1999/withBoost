#include "AlarmController.hpp"
#include <QDateTime>
#include <QVariantMap>

/**
 * @file AlarmController.cpp
 * @brief 알람 큐 관리 및 QML 연동 구현
 */

AlarmController::AlarmController(QObject *parent) : QObject(parent) {}

QVariantList AlarmController::alarms() const {
  QVariantList list;
  for (const auto &entry : alarms_) {
    QVariantMap map;
    map["id"] = entry.id;
    map["title"] = entry.title;
    map["detail"] = entry.detail;
    map["severity"] = entry.severity;
    list.append(map);
  }
  return list;
}

void AlarmController::dismiss(qint64 id) {
  for (int i = 0; i < alarms_.size(); ++i) {
    if (alarms_[i].id == id) {
      alarms_.removeAt(i);
      emit alarmsChanged();
      return;
    }
  }
}

void AlarmController::clearAll() {
  if (!alarms_.isEmpty()) {
    alarms_.clear();
    emit alarmsChanged();
  }
}

void AlarmController::onAlarm(QString title, QString detail, int severity) {
  AlarmEntry e;
  e.id = QDateTime::currentMSecsSinceEpoch();
  e.title = title;
  e.detail = detail;
  e.severity = severity;

  alarms_.prepend(e);
  if (alarms_.size() > kMaxAlarms) {
    alarms_.removeLast();
  }
  emit alarmsChanged();
}
/**
 * @section Workflow Guide
 *
 * **[AlarmController 구현 상세]**
 *
 * 1. ID 생성 전략:
 *    - `QDateTime::currentMSecsSinceEpoch()`와 정적 카운터를 조합하여 고유성을
 * 보장합니다.
 *    - 이는 별도의 DB 연동 없이도 런타임 중에 Dismiss 대상을 정확히 식별하기
 * 위함입니다.
 *
 * 2. 데이터 마샬링:
 *    - `alarms()` 메서드는 내부 `QList<AlarmEntry>`를 순회하며 `QVariantMap`
 * 객체들을 생성합니다.
 *    - 이는 C++ 구조체 데이터를 QML의 자바스크립트 객체와 동기화하기 위한 표준
 * 절차입니다.
 */
