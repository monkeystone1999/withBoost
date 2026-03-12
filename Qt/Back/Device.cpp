#include "Device.hpp"
#include <QDateTime>

DeviceModel::DeviceModel(QObject *parent) : QAbstractListModel(parent) {}

int DeviceModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return devices_.size();
}

QVariant DeviceModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= devices_.size())
    return {};

  const auto &d = devices_[index.row()];

  switch (role) {
  case IpRole:
    return d.ip;
  case RtspUrlRole:
    return d.rtspUrl;
  case TypeRole:
    return d.type;
  case IsOnlineRole:
    return d.isOnline;
  case CpuRole:
    return d.cpu;
  case MemoryRole:
    return d.memory;
  case TempRole:
    return d.temp;
  case UptimeRole:
    return d.uptime;
  case HasMotorRole:
    return d.hasMotor;
  case HasIrRole:
    return d.hasIr;
  case HasHeaterRole:
    return d.hasHeater;
  default:
    return {};
  }
}

QHash<int, QByteArray> DeviceModel::roleNames() const {
  return {{IpRole, "ip"},
          {RtspUrlRole, "rtspUrl"},
          {TypeRole, "type"},
          {IsOnlineRole, "isOnline"},
          {CpuRole, "cpu"},
          {MemoryRole, "memory"},
          {TempRole, "temp"},
          {UptimeRole, "uptime"},
          {HasMotorRole, "hasMotor"},
          {HasIrRole, "hasIr"},
          {HasHeaterRole, "hasHeater"}};
}

// ── onStoreUpdated ───────────────────────────────────────────────────────────
void DeviceModel::onStoreUpdated(std::vector<DeviceIntegrated> snapshot) {
  // IP 기반 업데이트
  for (const auto &snap : snapshot) {
    QString ip = QString::fromStdString(snap.ip);
    int idx = findIndexByIp(ip);

    if (idx >= 0) {
      // 기존 항목 업데이트
      auto &entry = devices_[idx];

      entry.rtspUrl = QString::fromStdString(snap.rtspUrl);
      entry.type = QString::fromStdString(snap.type);
      entry.isOnline = snap.isOnline;
      entry.cpu = snap.cpu;
      entry.memory = snap.memory;
      entry.temp = snap.temp;
      entry.uptime = snap.uptime;
      entry.lastUpdate = snap.lastUpdate;
      entry.hasMotor = snap.hasMotor;
      entry.hasIr = snap.hasIr;
      entry.hasHeater = snap.hasHeater;

      // History 업데이트
      entry.history.clear();
      for (const auto &h : snap.history) {
        entry.history.append(h);
      }

      emit dataChanged(createIndex(idx, 0), createIndex(idx, 0));

    } else {
      // 신규 항목 추가
      DeviceEntry entry;
      entry.ip = ip;
      entry.rtspUrl = QString::fromStdString(snap.rtspUrl);
      entry.type = QString::fromStdString(snap.type);
      entry.isOnline = snap.isOnline;
      entry.cpu = snap.cpu;
      entry.memory = snap.memory;
      entry.temp = snap.temp;
      entry.uptime = snap.uptime;
      entry.lastUpdate = snap.lastUpdate;
      entry.hasMotor = snap.hasMotor;
      entry.hasIr = snap.hasIr;
      entry.hasHeater = snap.hasHeater;

      for (const auto &h : snap.history) {
        entry.history.append(h);
      }

      beginInsertRows(QModelIndex(), devices_.size(), devices_.size());
      devices_.append(entry);
      endInsertRows();
    }
  }

  // Rebuild byIp_
  byIp_.clear();
  for (int i = 0; i < devices_.size(); ++i) {
    byIp_[devices_[i].ip] = i;
  }
}

// ── QML 조회 함수 ────────────────────────────────────────────────────────────
int DeviceModel::findIndexByIp(const QString &ip) const {
  auto it = byIp_.find(ip);
  return it != byIp_.end() ? *it : -1;
}

bool DeviceModel::hasDevice(const QString &ip) const {
  return findIndexByIp(ip) >= 0;
}

QString DeviceModel::rtspUrl(const QString &ip) const {
  int idx = findIndexByIp(ip);
  return idx >= 0 ? devices_[idx].rtspUrl : "";
}

double DeviceModel::cpu(const QString &ip) const {
  int idx = findIndexByIp(ip);
  return idx >= 0 ? devices_[idx].cpu : 0.0;
}

double DeviceModel::memory(const QString &ip) const {
  int idx = findIndexByIp(ip);
  return idx >= 0 ? devices_[idx].memory : 0.0;
}

double DeviceModel::temp(const QString &ip) const {
  int idx = findIndexByIp(ip);
  return idx >= 0 ? devices_[idx].temp : 0.0;
}

// ✅ History 조회 (QML용 QVariantList 반환)
QVariantList DeviceModel::getHistory(const QString &ip) const {
  int idx = findIndexByIp(ip);
  if (idx < 0)
    return {};

  QVariantList result;
  for (const auto &h : devices_[idx].history) {
    QVariantMap item;
    item["timestamp"] = h.timestamp;
    item["cpu"] = h.cpu;
    item["memory"] = h.memory;
    item["temp"] = h.temp;
    item["uptime"] = h.uptime;
    result.append(item);
  }

  return result;
}

// --- rtspUrl-based lookup methods ---

int DeviceModel::findIndexByRtspUrl(const QString &rtspUrl) const {
  for (int i = 0; i < devices_.size(); ++i) {
    if (devices_[i].rtspUrl == rtspUrl)
      return i;
  }
  return -1;
}

QString DeviceModel::deviceIp(const QString &rtspUrl) const {
  int idx = findIndexByRtspUrl(rtspUrl);
  return idx >= 0 ? devices_[idx].ip : QString();
}

bool DeviceModel::hasMotor(const QString &rtspUrl) const {
  int idx = findIndexByRtspUrl(rtspUrl);
  return idx >= 0 ? devices_[idx].hasMotor : false;
}

bool DeviceModel::hasIr(const QString &rtspUrl) const {
  int idx = findIndexByRtspUrl(rtspUrl);
  return idx >= 0 ? devices_[idx].hasIr : false;
}

bool DeviceModel::hasHeater(const QString &rtspUrl) const {
  int idx = findIndexByRtspUrl(rtspUrl);
  return idx >= 0 ? devices_[idx].hasHeater : false;
}

bool DeviceModel::hasDeviceByUrl(const QString &rtspUrl) const {
  return findIndexByRtspUrl(rtspUrl) >= 0;
}
