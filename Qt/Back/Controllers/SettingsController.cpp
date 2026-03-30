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

/**
 * @section Workflow Guide
 *
 * **[SettingsController 구현 세부 로직]**
 *
 * 1. 하이브리드 설정 소스:
 *    - `QSettings`를 사용하여 레지스트리 또는 OS 표준 위치에 사용자 환경 설정을
 * 저장함으로써 앱 재시작 시에도 테마 등이 유지되도록 합니다.
 *    - 고정된 시스템 인프라 정보(`ServerIP`, `FPS`)는 실행 파일 경로의
 * `Settings.json`을 직접 파싱하여 획득합니다.
 *
 * 2. 예외 처리:
 *    - JSON 파일이 없거나 FPS 값이 비정상적(<= 0)일 경우, 하드코딩된 기본값(30
 * FPS)을 안전하게 복구(Fallback)합니다.
 *
 * 3. QML 프로퍼티 통지:
 *    - JSON 데이터 로드 시에도 `emit` 시그널을 호출하여, 앱 초기화 단계에서
 * UI가 즉시 올바른 서버 정보를 표시하도록 보장합니다.
 */
