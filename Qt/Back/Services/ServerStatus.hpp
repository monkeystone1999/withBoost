#pragma once
#include <string>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <vector>

struct ServerStatusDeviceData {
    std::string ip;
    double cpu = 0, memory = 0, temp = 0;
    int uptime = 0, pendingEvents = 0;
};
struct ServerStatusData {
    double cpu = 0, memory = 0, temp = 0;
    int uptime = 0;
    bool available = false;
    long long timestamp = 0;
    std::vector<ServerStatusDeviceData> devices;
};

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
  long long timestamp = 0;
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

  double serverCpu() const { return server_.cpu; }
  double serverMemory() const { return server_.memory; }
  double serverTemp() const { return server_.temp; }
  bool hasServer() const { return server_.available; }

  // QML에서 ip로 디바이스 상태 조회
  Q_INVOKABLE double deviceCpu(const QString &ip) const;
  Q_INVOKABLE double deviceMemory(const QString &ip) const;
  Q_INVOKABLE double deviceTemp(const QString &ip) const;
  Q_INVOKABLE int deviceUptime(const QString &ip) const;

  // QML에서 서버 히스토리 반환 (Canvas Graph용)
  Q_INVOKABLE QVariantList getServerHistory() const;

public slots:
  // Called by Core (GUI thread) with pre-parsed snapshot — no JSON
  void onStoreUpdated(ServerStatusData data);

signals:
  void statusUpdated();

private:
  ServerStatus server_;
  QList<DeviceStatus> devices_;
  QVariantList serverHistory_;
};
