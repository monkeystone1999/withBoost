#pragma once
#include <QObject>
#include <QVariantList>
#include <QtTypes>

// ============================================================
//  AlarmController — 알람 큐 관리 (C++)
//
//  이전에 Main.qml alarmLayer JS에 있던:
//    - addAlarm / removeAlarm
//    - pendingQueue (화면 초과 대기)
//    - totalDisplayedHeight 트래킹
//  를 C++로 이전.
//
//  QML은 alarmController.alarms를 ListView에 바인딩하기만 함.
// ============================================================
class AlarmController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList alarms READ alarms NOTIFY alarmsChanged)

public:
  static constexpr int kMaxAlarms = 20;

  explicit AlarmController(QObject *parent = nullptr);

  QVariantList alarms() const;

  // QML에서 카드 닫기 버튼 클릭 시 호출
  Q_INVOKABLE void dismiss(qint64 id);

  // 모든 알람 초기화
  Q_INVOKABLE void clearAll();

public slots:
  // AlarmManager::alarmTriggered에 연결
  void onAlarmTriggered(const QString &title, const QString &detail,
                        int severity);

signals:
  void alarmsChanged();

private:
  struct AlarmEntry {
    qint64 id;
    QString title;
    QString detail;
    int severity; // 0=info, 1=warning, 2=critical
  };

  QList<AlarmEntry> alarms_;
};
