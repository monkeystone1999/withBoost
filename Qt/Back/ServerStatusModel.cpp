#include "ServerStatusModel.hpp"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>


ServerStatusModel::ServerStatusModel(QObject *parent) : QObject(parent) {}

void ServerStatusModel::refreshFromJson(const QString &jsonString) {
  QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
  if (!doc.isObject())
    return;

  const QJsonObject root = doc.object();

  // devices 배열 파싱
  m_devices.clear();
  const QJsonArray devArr = root["devices"].toArray();
  for (const auto &val : devArr) {
    const QJsonObject obj = val.toObject();
    DeviceStatus s;
    s.ip = obj["ip"].toString();
    s.cpu = obj["cpu"].toDouble();
    s.memory = obj["memory"].toDouble();
    s.temp = obj["temp"].toDouble();
    s.uptime = obj["uptime"].toInt();
    s.pendingEvents = obj["pending_events"].toInt();
    m_devices.append(s);
  }

  // server 키는 admin에게만 있음
  if (root.contains("server")) {
    const QJsonObject srv = root["server"].toObject();
    m_server.cpu = srv["cpu"].toDouble();
    m_server.memory = srv["memory"].toDouble();
    m_server.temp = srv["temp"].toDouble();
    m_server.uptime = srv["uptime"].toInt();
    m_server.available = true;
  } else {
    m_server = ServerStatus{}; // user는 server 상태 없음
  }

  qDebug() << "[ServerStatusModel] updated, devices:" << m_devices.size()
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
