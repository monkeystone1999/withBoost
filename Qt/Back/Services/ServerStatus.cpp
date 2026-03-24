#include "ServerStatus.hpp"

ServerStatusModel::ServerStatusModel(QObject *parent) : QObject(parent) {}

// ── New path: called by Core via QMetaObject::invokeMethod on GUI thread
// ────── Receives a pre-parsed ServerStatusData from ServerStatusStore
// (ThreadPool). NO JSON work here.
void ServerStatusModel::onStoreUpdated(ServerStatusData data) {
  devices_.clear();
  for (const auto &d : data.devices) {
    DeviceStatus s;
    s.ip = QString::fromStdString(d.ip);
    s.cpu = d.cpu;
    s.memory = d.memory;
    s.temp = d.temp;
    s.uptime = d.uptime;
    s.pendingEvents = d.pendingEvents;
    devices_.append(s);
  }

  server_.cpu = data.cpu;
  server_.memory = data.memory;
  server_.temp = data.temp;
  server_.uptime = data.uptime;
  server_.available = data.available;
  server_.timestamp = data.timestamp;

  if (server_.available) {
    QVariantMap item;
    item["timestamp"] = server_.timestamp;
    item["cpu"] = server_.cpu;
    item["memory"] = server_.memory;
    item["temp"] = server_.temp;
    item["uptime"] = server_.uptime;

    serverHistory_.append(QVariant(item));
    if (serverHistory_.size() > 60) {
      serverHistory_.removeFirst();
    }
  }

  emit statusUpdated();
}

double ServerStatusModel::deviceCpu(const QString &ip) const {
  for (const auto &d : devices_)
    if (d.ip == ip)
      return d.cpu;
  return 0;
}
double ServerStatusModel::deviceMemory(const QString &ip) const {
  for (const auto &d : devices_)
    if (d.ip == ip)
      return d.memory;
  return 0;
}
double ServerStatusModel::deviceTemp(const QString &ip) const {
  for (const auto &d : devices_)
    if (d.ip == ip)
      return d.temp;
  return 0;
}
int ServerStatusModel::deviceUptime(const QString &ip) const {
  for (const auto &d : devices_)
    if (d.ip == ip)
      return d.uptime;
  return 0;
}

QVariantList ServerStatusModel::getServerHistory() const {
  return serverHistory_;
}
