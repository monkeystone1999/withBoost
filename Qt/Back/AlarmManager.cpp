#include "AlarmManager.hpp"
#include <QDebug>

AlarmManager::AlarmManager(AlarmDispatcher *dispatcher, QObject *parent)
    : QObject(parent), m_dispatcher(dispatcher) {}

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

// ── Legacy path: direct-connect from NetworkBridge::aiResultReceived
// ────────── Immediately dispatches raw JSON to the ThreadPool via
// AlarmDispatcher. The ThreadPool callback then calls Core → onAlarm (above)
// via invokeMethod.
void AlarmManager::onAiJson(const QString &json) {
  if (!m_dispatcher)
    return;
  const std::string s = json.toStdString();
  // AlarmDispatcher internally submits to ThreadPool and invokes callback.
  // Core wires the callback → QMetaObject::invokeMethod → onAlarm.
  m_dispatcher->dispatch(s, [this](AlarmEvent ev) {
    QMetaObject::invokeMethod(
        this, [this, ev = std::move(ev)]() { onAlarm(std::move(ev)); },
        Qt::QueuedConnection);
  });
}
