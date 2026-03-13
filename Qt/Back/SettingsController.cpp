#include "SettingsController.hpp"
#include <QSettings>

SettingsController::SettingsController(QObject *parent) : QObject(parent) {
  load();
}

void SettingsController::setDarkMode(bool v) {
  if (darkMode_ == v)
    return;
  darkMode_ = v;
  emit darkModeChanged();
}

void SettingsController::setAlarmSound(bool v) {
  if (alarmSound_ == v)
    return;
  alarmSound_ = v;
  emit alarmSoundChanged();
}

void SettingsController::setDefaultGrid(int v) {
  if (defaultGrid_ == v)
    return;
  defaultGrid_ = v;
  emit defaultGridChanged();
}

void SettingsController::setLogLevel(const QString &v) {
  if (logLevel_ == v)
    return;
  logLevel_ = v;
  emit logLevelChanged();
}

void SettingsController::save() {
  QSettings s("AnoMap", "AnoMap");
  s.setValue("ui/darkMode", darkMode_);
  s.setValue("ui/alarmSound", alarmSound_);
  s.setValue("ui/defaultGrid", defaultGrid_);
  s.setValue("ui/logLevel", logLevel_);
}

void SettingsController::load() {
  QSettings s("AnoMap", "AnoMap");
  darkMode_ = s.value("ui/darkMode", true).toBool();
  alarmSound_ = s.value("ui/alarmSound", true).toBool();
  defaultGrid_ = s.value("ui/defaultGrid", 4).toInt();
  logLevel_ = s.value("ui/logLevel", "INFO").toString();
}
