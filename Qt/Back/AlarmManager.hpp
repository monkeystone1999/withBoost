#pragma once

#include <QObject>
#include <QString>

class AlarmManager : public QObject {
  Q_OBJECT
public:
  explicit AlarmManager(QObject *parent = nullptr);
  ~AlarmManager() override;

public slots:
  void parseAiMessage(const QString &jsonString);

signals:
  void alarmTriggered(QString title, QString detail, int severity);
};
