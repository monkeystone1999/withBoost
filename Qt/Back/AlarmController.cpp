#include "AlarmController.hpp"
#include <QDateTime>
#include <QVariantMap>

AlarmController::AlarmController(QObject *parent) : QObject(parent) {}

QVariantList AlarmController::alarms() const {
  QVariantList list;
  for (const auto &e : alarms_) {
    QVariantMap m;
    m["id"] = e.id;
    m["title"] = e.title;
    m["detail"] = e.detail;
    m["severity"] = e.severity;
    list.append(m);
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
  if (alarms_.isEmpty())
    return;
  alarms_.clear();
  emit alarmsChanged();
}

void AlarmController::onAlarmTriggered(const QString &title,
                                       const QString &detail, int severity) {
  // 최대 kMaxAlarms개 유지 (오래된 것부터 제거)
  while (alarms_.size() >= kMaxAlarms) {
    alarms_.removeFirst();
  }

  AlarmEntry e;
  e.id = QDateTime::currentMSecsSinceEpoch();
  e.title = title;
  e.detail = detail;
  e.severity = severity;
  alarms_.append(e);
  emit alarmsChanged();
}
