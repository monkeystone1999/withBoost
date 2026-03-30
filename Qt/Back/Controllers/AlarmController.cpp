#include "AlarmController.hpp"
#include "Domain/Alarm.hpp"
#include <QDateTime>
#include <QVariantMap>
#include <algorithm>

/**
 * @file AlarmController.cpp
 * @brief Alarm UI ViewModel implementation.
 *
 * This class translates domain AlarmEvent (C++) into a QVariantList (QML)
 * and manages the visual queue (FIFO, max 20 entries).
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
  auto it = std::find_if(alarms_.begin(), alarms_.end(),
                         [id](const AlarmEntry &e) { return e.id == id; });

  if (it != alarms_.end()) {
    alarms_.erase(it);
    emit alarmsChanged();
  }
}

void AlarmController::clearAll() {
  if (!alarms_.isEmpty()) {
    alarms_.clear();
    emit alarmsChanged();
  }
}

void AlarmController::onAlarm(AlarmEvent ev) {
  // Maintain FIFO queue of size kMaxAlarms
  if (alarms_.size() >= kMaxAlarms) {
    alarms_.removeFirst();
  }

  AlarmEntry entry;
  // Use timestamp as a simple unique ID for QML delegate tracking
  entry.id = QDateTime::currentMSecsSinceEpoch();
  entry.title = QString::fromStdString(ev.title);
  entry.detail = QString::fromStdString(ev.detail);
  entry.severity = ev.severity;

  alarms_.append(entry);
  emit alarmsChanged();
}
