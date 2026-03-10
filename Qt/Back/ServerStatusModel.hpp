#pragma once
#include "../../Src/Domain/ServerStatusData.hpp"
#include <QList>
#include <QObject>
#include <QString>
#include <vector>

struct DeviceStatus {
  QString ip;
  double cpu = 0;
  double memory = 0;
  double temp = 0;
  int uptime = 0;
  int pendingEvents = 0;
};

struct ServerStatus {
  double cpu = 0;
  double memory = 0;
  double temp = 0;
  int uptime = 0;
  bool available = false; // "server" 키가 있을 때만 true (admin)
};

class ServerStatusModel : public QObject {
  Q_OBJECT
  // QML 프로퍼티
  Q_PROPERTY(double serverCpu READ serverCpu NOTIFY statusUpdated)
  Q_PROPERTY(double serverMemory READ serverMemory NOTIFY statusUpdated)
  Q_PROPERTY(double serverTemp READ serverTemp NOTIFY statusUpdated)
  Q_PROPERTY(bool hasServer READ hasServer NOTIFY statusUpdated)
public:
  explicit ServerStatusModel(QObject *parent = nullptr);

  double serverCpu() const { return m_server.cpu; }
  double serverMemory() const { return m_server.memory; }
  double serverTemp() const { return m_server.temp; }
  bool hasServer() const { return m_server.available; }

  // QML에서 ip로 디바이스 상태 조회
  Q_INVOKABLE double deviceCpu(const QString &ip) const;
  Q_INVOKABLE double deviceMemory(const QString &ip) const;
  Q_INVOKABLE double deviceTemp(const QString &ip) const;
  Q_INVOKABLE int deviceUptime(const QString &ip) const;

public slots:
  // Called by Core (GUI thread) with pre-parsed snapshot — no JSON
  void onStoreUpdated(ServerStatusData data);

signals:
  void statusUpdated();

private:
  ServerStatus m_server;
  QList<DeviceStatus> m_devices;
};
