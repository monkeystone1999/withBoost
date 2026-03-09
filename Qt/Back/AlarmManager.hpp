#pragma once

#include "../../Src/Domain/AlarmDispatcher.hpp"
#include "../../Src/Domain/AlarmEvent.hpp"
#include <QObject>
#include <QString>

// ============================================================
//  AlarmManager — Qt adapter for alarm notifications
//
//  ARCHITECTURE.md §4.10:
//    - Receives pre-parsed AlarmEvent from Core (GUI thread, post-ThreadPool)
//    - onAlarm() is called via QMetaObject::invokeMethod(Qt::QueuedConnection)
//    - Does NOT parse JSON. Does NOT own the ThreadPool.
// ============================================================

class AlarmManager : public QObject {
  Q_OBJECT
public:
  explicit AlarmManager(AlarmDispatcher *dispatcher, QObject *parent = nullptr);
  ~AlarmManager() override;

public slots:
  // New path: called by Core on the GUI thread with a pre-parsed event
  void onAlarm(AlarmEvent ev);

  // Legacy: direct-connect from NetworkBridge::aiResultReceived
  // Immediately dispatches raw JSON to the ThreadPool via AlarmDispatcher
  void onAiJson(const QString &json);

signals:
  void alarmTriggered(QString title, QString detail, int severity);

private:
  AlarmDispatcher *m_dispatcher; // non-owning
};
