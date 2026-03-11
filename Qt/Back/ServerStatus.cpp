#include "ServerStatus.hpp"
#include <QDebug>

ServerStatusModel::ServerStatusModel(QObject *parent) : QObject(parent) {}

// ── New path: called by Core via QMetaObject::invokeMethod on GUI thread
// ────── Receives a pre-parsed ServerStatusData from ServerStatusStore
// (ThreadPool). NO JSON work here.
void ServerStatusModel::onStoreUpdated(ServerStatusData data) {
  m_devices.clear();
  for (const auto &d : data.devices) {
    DeviceStatus s;
    s.ip = QString::fromStdString(d.ip);
    s.cpu = d.cpu;
    s.memory = d.memory;
    s.temp = d.temp;
    s.uptime = d.uptime;
    s.pendingEvents = d.pendingEvents;
    m_devices.append(s);
  }

  m_server.cpu = data.cpu;
  m_server.memory = data.memory;
  m_server.temp = data.temp;
  m_server.uptime = data.uptime;
  m_server.available = data.available;

  qDebug() << "[ServerStatusModel] onStoreUpdated, devices:" << m_devices.size()
           << "hasServer:" << m_server.available;

  emit statusUpdated();
}

double ServerStatusModel::deviceCpu(const QString &ip) const {
  for (const auto &d : m_devices)
    if (d.ip == ip)
      return d.cpu;
  return 0;
}
double ServerStatusModel::deviceMemory(const QString &ip) const {
  for (const auto &d : m_devices)
    if (d.ip == ip)
      return d.memory;
  return 0;
}
double ServerStatusModel::deviceTemp(const QString &ip) const {
  for (const auto &d : m_devices)
    if (d.ip == ip)
      return d.temp;
  return 0;
}
int ServerStatusModel::deviceUptime(const QString &ip) const {
  for (const auto &d : m_devices)
    if (d.ip == ip)
      return d.uptime;
  return 0;
}
