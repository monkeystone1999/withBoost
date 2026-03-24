#include "SettingsController.hpp"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

void SettingsController::setDefaultResolution(const QString &v) {
  if (defaultResolution_ == v)
    return;
  defaultResolution_ = v;
  emit defaultResolutionChanged();
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
  darkMode_ = s.value("ui/darkMode", false).toBool();
  alarmSound_ = s.value("ui/alarmSound", true).toBool();
  defaultGrid_ = s.value("ui/defaultGrid", 4).toInt();
  logLevel_ = s.value("ui/logLevel", "INFO").toString();

  // Load Settings.json
  QString jsonPath = "Settings.json";
  if (!QFile::exists(jsonPath)) {
    jsonPath =
        QDir(QCoreApplication::applicationDirPath()).filePath("Settings.json");
  }

  QFile file(jsonPath);
  if (file.open(QIODevice::ReadOnly)) {
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
      QJsonObject obj = doc.object();
      if (obj.contains("ServerIP")) {
        serverIp_ = obj["ServerIP"].toString();
        emit serverIpChanged();
      }
      if (obj.contains("FPS")) {
        fps_ = obj["FPS"].toString().toInt();
        if (fps_ <= 0)
          fps_ = 30;
        emit fpsChanged();
      }
      if (obj.contains("Resolution")) {
        QJsonArray arr = obj["Resolution"].toArray();
        resolutions_.clear();
        for (int i = 0; i < arr.size(); ++i) {
          resolutions_.append(arr[i].toString());
        }
      }
    }
    file.close();
  }

  defaultResolution_ =
      s.value("ui/defaultResolution",
              resolutions_.isEmpty() ? "192.168.0.58" : resolutions_.first())
          .toString();
  if (!resolutions_.contains(defaultResolution_) && !resolutions_.isEmpty()) {
    defaultResolution_ = resolutions_.first();
  }
}
