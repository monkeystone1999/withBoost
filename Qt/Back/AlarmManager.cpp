#include "AlarmManager.hpp"
#include <QDebug>

AlarmManager::AlarmManager(AlarmDispatcher *dispatcher, QObject *parent)
    : QObject(parent), dispatcher_(dispatcher) {}

AlarmManager::~AlarmManager() {}

// ── New path (§4.10): called by Core on GUI thread after ThreadPool parse
// ───── The AlarmDispatcher already parsed the JSON on the ThreadPool. This
// slot just converts std::string → QString and emits the signal.
void AlarmManager::onAlarm(AlarmEvent ev) {
  qDebug() << "[AlarmManager] onAlarm: severity=" << ev.severity
           << "title=" << QString::fromStdString(ev.title);
  emit alarmTriggered(QString::fromStdString(ev.title),
                      QString::fromStdString(ev.detail), ev.severity);
}
