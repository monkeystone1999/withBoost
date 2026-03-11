#include "Device.hpp"
#include <QDebug>
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

// ── New path: called by Core via QMetaObject::invokeMethod on GUI thread
// ────── Receives a pre-parsed vector<DeviceData> from DeviceStore
// (ThreadPool). NO JSON work here. Just update the Qt model.
void DeviceModel::onStoreUpdated(std::vector<DeviceData> snapshot) {
  QSet<QString> representedUrls;

  // 1. Update existing entries
  for (int i = 0; i < m_devices.size(); ++i) {
    auto &entry = m_devices[i];
    auto it = std::find_if(
        snapshot.begin(), snapshot.end(), [&](const DeviceData &d) {
          return QString::fromStdString(d.rtspUrl) == entry.rtspUrl;
        });

    bool changed = false;
    QList<int> changedRoles;

    if (it == snapshot.end()) {
      // Mark offline if gone
      if (entry.isOnline) {
        entry.isOnline = false;
        changed = true;
        changedRoles << IsOnlineRole;
      }
    } else {
      const DeviceData &snap = *it;
      representedUrls.insert(entry.rtspUrl);

      if (entry.isOnline != snap.isOnline) {
        entry.isOnline = snap.isOnline;
        changed = true;
        changedRoles << IsOnlineRole;
      }

      QString snapIp = QString::fromStdString(snap.deviceIp);
      if (entry.deviceIp != snapIp) {
        entry.deviceIp = snapIp;
        changed = true;
        changedRoles << DeviceIpRole;
      }

      QString snapTitle = QString::fromStdString(snap.title);
      if (entry.title != snapTitle) {
        entry.title = snapTitle;
        changed = true;
        changedRoles << TitleRole;
      }

      if (entry.cap.motor != snap.cap.motor) {
        entry.cap.motor = snap.cap.motor;
        changed = true;
        changedRoles << HasMotorRole;
      }
      if (entry.cap.ir != snap.cap.ir) {
        entry.cap.ir = snap.cap.ir;
        changed = true;
        changedRoles << HasIrRole;
      }
      if (entry.cap.heater != snap.cap.heater) {
        entry.cap.heater = snap.cap.heater;
        changed = true;
        changedRoles << HasHeaterRole;
      }
    }

    if (changed) {
      emit dataChanged(createIndex(i, 0), createIndex(i, 0), changedRoles);
    }
  }

  // 2. Insert new entries
  QList<DeviceEntry> newEntries;
  int nextIdx = m_devices.size();
  for (const auto &d : snapshot) {
    QString url = QString::fromStdString(d.rtspUrl);
    if (representedUrls.contains(url))
      continue;

    DeviceEntry e;
    e.rtspUrl = url;
    e.deviceIp = QString::fromStdString(d.deviceIp);
    e.title = QString::fromStdString(d.title);
    e.isOnline = d.isOnline;
    e.cap.motor = d.cap.motor;
    e.cap.ir = d.cap.ir;
    e.cap.heater = d.cap.heater;

    newEntries.append(e);
  }

  if (!newEntries.isEmpty()) {
    beginInsertRows(QModelIndex(), m_devices.size(),
                    m_devices.size() + newEntries.size() - 1);
    for (const auto &e : newEntries) {
      m_devices.append(e);
    }
    endInsertRows();
  }

  // Rebuild m_byUrl fully here to prevent stale indices
  m_byUrl.clear();
  for (int i = 0; i < m_devices.size(); ++i) {
    if (!m_devices[i].rtspUrl.isEmpty()) {
      m_byUrl[m_devices[i].rtspUrl] = i;
    }
  }

  qDebug() << "[DeviceModel] onStoreUpdated, count:" << m_devices.size();
}
