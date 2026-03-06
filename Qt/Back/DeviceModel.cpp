#include "DeviceModel.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace {
QString hostFromRtsp(const QString &rtspUrl) {
  const QUrl url(rtspUrl);
  if (url.isValid() && !url.host().isEmpty())
    return url.host();

  // Fallback parser for malformed URLs.
  const int schemePos = rtspUrl.indexOf("://");
  if (schemePos < 0)
    return {};
  const int hostStart = schemePos + 3;
  int atPos = rtspUrl.indexOf('@', hostStart);
  int hostPos = (atPos >= 0) ? atPos + 1 : hostStart;
  int endPos = rtspUrl.indexOf('/', hostPos);
  if (endPos < 0)
    endPos = rtspUrl.size();
  const QString hostPort = rtspUrl.mid(hostPos, endPos - hostPos);
  const int colonPos = hostPort.indexOf(':');
  return colonPos >= 0 ? hostPort.left(colonPos) : hostPort;
}
} // namespace

DeviceModel::DeviceModel(QObject *parent) : QAbstractListModel(parent) {}

int DeviceModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_devices.size();
}

QVariant DeviceModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_devices.size())
    return {};
  const auto &d = m_devices[index.row()];
  switch (role) {
  case RtspUrlRole:
    return d.rtspUrl;
  case DeviceIpRole:
    return d.deviceIp;
  case TitleRole:
    return d.title;
  case IsOnlineRole:
    return d.isOnline;
  case HasMotorRole:
    return d.cap.motor;
  case HasIrRole:
    return d.cap.ir;
  case HasHeaterRole:
    return d.cap.heater;
  default:
    return {};
  }
}

QHash<int, QByteArray> DeviceModel::roleNames() const {
  return {
      {RtspUrlRole, "rtspUrl"},     {DeviceIpRole, "deviceIp"},
      {TitleRole, "title"},         {IsOnlineRole, "isOnline"},
      {HasMotorRole, "hasMotor"},   {HasIrRole, "hasIr"},
      {HasHeaterRole, "hasHeater"},
  };
}

bool DeviceModel::hasDevice(const QString &url) const {
  return findIndexByRtspUrl(url) >= 0;
}
bool DeviceModel::hasMotor(const QString &url) const {
  const int idx = findIndexByRtspUrl(url);
  return idx >= 0 ? m_devices[idx].cap.motor : false;
}
bool DeviceModel::hasIr(const QString &url) const {
  const int idx = findIndexByRtspUrl(url);
  return idx >= 0 ? m_devices[idx].cap.ir : false;
}
bool DeviceModel::hasHeater(const QString &url) const {
  const int idx = findIndexByRtspUrl(url);
  return idx >= 0 ? m_devices[idx].cap.heater : false;
}
QString DeviceModel::deviceIp(const QString &url) const {
  const int idx = findIndexByRtspUrl(url);
  return idx >= 0 ? m_devices[idx].deviceIp : "";
}

int DeviceModel::findIndexByRtspUrl(const QString &rtspUrl) const {
  const auto it = m_byUrl.find(rtspUrl);
  if (it != m_byUrl.end())
    return *it;

  const QString host = hostFromRtsp(rtspUrl);
  if (host.isEmpty())
    return -1;

  for (int i = 0; i < m_devices.size(); ++i) {
    const auto &d = m_devices[i];
    if (!d.deviceIp.isEmpty() && d.deviceIp == host)
      return i;
    if (hostFromRtsp(d.rtspUrl) == host)
      return i;
  }
  return -1;
}

// 서버 DEVICE 패킷 body JSON 포맷 (배열):
// [
//   {
//     "rtsp_url":   "rtsp://...",
//     "device_ip":  "192.168.0.23",
//     "title":      "PTZ-1",
//     "is_online":  true,
//     "capabilities": { "motor": true, "ir": false, "heater": false }
//   }, ...
// ]
void DeviceModel::refreshFromJson(const QString &jsonString) {
  QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
  if (!doc.isArray())
    return;

  beginResetModel();
  m_devices.clear();
  m_byUrl.clear();

  const QJsonArray arr = doc.array();
  for (int i = 0; i < arr.size(); ++i) {
    const QJsonObject obj = arr[i].toObject();
    const QJsonObject caps = obj["capabilities"].toObject();
    DeviceEntry e;
    e.rtspUrl = obj["rtsp_url"].toString();
    if (e.rtspUrl.isEmpty())
      e.rtspUrl = obj["source_url"].toString();

    e.deviceIp = obj["device_ip"].toString();
    if (e.deviceIp.isEmpty())
      e.deviceIp = obj["ip"].toString();
    if (e.deviceIp.isEmpty())
      e.deviceIp = hostFromRtsp(e.rtspUrl);

    e.title = obj["title"].toString();
    if (e.title.isEmpty())
      e.title = e.deviceIp;
    e.isOnline = obj["is_online"].toBool();
    e.cap.motor = caps["motor"].toBool(true); // motor 는 기본 true
    e.cap.ir = caps["ir"].toBool(false);
    e.cap.heater = caps["heater"].toBool(false);
    if (!e.rtspUrl.isEmpty())
      m_byUrl[e.rtspUrl] = i;
    m_devices.append(e);
  }

  endResetModel();
  qDebug() << "[DeviceModel] refreshed, count:" << m_devices.size();
}
