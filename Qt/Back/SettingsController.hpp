#pragma once
#include <QObject>
#include <QString>

// ============================================================
//  SettingsController — 앱 설정 영속화 (QSettings 기반)
//
//  이전에 Main.qml OptionDialog에 TODO로 남아있던:
//    - darkMode / alarmSound / defaultGrid / logLevel
//  를 C++에서 load/save.
//
//  Theme.isDark ↔ settingsController.darkMode 양방향 바인딩.
// ============================================================
class SettingsController : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
  Q_PROPERTY(bool alarmSound READ alarmSound WRITE setAlarmSound NOTIFY
                 alarmSoundChanged)
  Q_PROPERTY(int defaultGrid READ defaultGrid WRITE setDefaultGrid NOTIFY
                 defaultGridChanged)
  Q_PROPERTY(
      QString logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)

public:
  explicit SettingsController(QObject *parent = nullptr);

  bool darkMode() const { return darkMode_; }
  bool alarmSound() const { return alarmSound_; }
  int defaultGrid() const { return defaultGrid_; }
  QString logLevel() const { return logLevel_; }

  void setDarkMode(bool v);
  void setAlarmSound(bool v);
  void setDefaultGrid(int v);
  void setLogLevel(const QString &v);

  Q_INVOKABLE void save(); // QSettings에 저장
  Q_INVOKABLE void load(); // QSettings에서 로드

signals:
  void darkModeChanged();
  void alarmSoundChanged();
  void defaultGridChanged();
  void logLevelChanged();

private:
  bool darkMode_{true};
  bool alarmSound_{true};
  int defaultGrid_{4};
  QString logLevel_{"INFO"};
};
