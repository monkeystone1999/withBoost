#include "AlarmManager.hpp"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

AlarmManager::AlarmManager(QObject *parent) : QObject(parent) {}

AlarmManager::~AlarmManager() {}

void AlarmManager::parseAiMessage(const QString &jsonString) {
  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);

  if (error.error != QJsonParseError::NoError) {
    qWarning() << "Failed to parse AI message JSON:" << error.errorString();
    return;
  }

  if (doc.isObject()) {
    QJsonObject obj = doc.object();

    // Check if the AI payload explicitly requested firing an alarm
    // The exact JSON structure logic depends on the C++ side protocol
    // definition, assuming default structure: { "type": "alarm", "title":
    // "...", "detail": "...", "severity": 2 }
    if (obj.contains("type") && obj["type"].toString() == "alarm") {
      QString title =
          obj.contains("title") ? obj["title"].toString() : "AI Alert";
      QString detail = obj.contains("detail") ? obj["detail"].toString()
                                              : "Anomaly detected";
      int severity = obj.contains("severity") ? obj["severity"].toInt() : 1;

      emit alarmTriggered(title, detail, severity);
    }
  }
}
