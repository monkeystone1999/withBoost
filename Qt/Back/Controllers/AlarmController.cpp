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

void AlarmController::onAlarm(AlarmEvent ev) {
  AlarmEntry entry;
  // 고유 ID 생성 (타임스탬프 기반)
  static qint64 counter = 0;
  entry.id = QDateTime::currentMSecsSinceEpoch() + (counter++ % 1000);

  entry.title = QString::fromStdString(ev.title);
  entry.detail = QString::fromStdString(ev.detail);
  entry.severity = ev.severity;

  // 최신 알람을 상단에 추가
  alarms_.prepend(entry);

  // 최대 개수 유지 (kMaxAlarms = 20)
  while (alarms_.size() > kMaxAlarms) {
    alarms_.removeLast();
  }

  emit alarmsChanged();
}
