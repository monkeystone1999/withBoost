#include "DeviceModel.hpp"
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
  case CameraIdRole:
    return d.cameraId;
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
          {CameraIdRole, "cameraId"},
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

// ── refreshFromCameraManager
// ───────────────────────────────────────────────────────────
void DeviceModel::refreshFromCameraManager() {
  if (!cameraManager_)
    return;
  beginResetModel();
  devices_.clear();
  byIp_.clear();

  for (auto &[id, info] : cameraManager_->getCameras()) {
    QString ip = QString::fromStdString(info.ip_);
    QString cid =
        QString::fromStdString(info.ip_ + "/" + std::to_string(info.index_));

    DeviceEntry entry;
    entry.ip = ip;
    entry.cameraId = cid;
    entry.isOnline = true;
    entry.temp = info.MetaData_.tmp_.empty() ? 0.0 : info.MetaData_.tmp_.back();
    // hum/light/tilt map to device sensors
    entry.hasMotor = true;
    entry.hasIr = info.Status_.ir_on;
    entry.hasHeater = info.Status_.heater_on;

    devices_.append(entry);
    byIp_[ip] = devices_.size() - 1;
  }
  endResetModel();
}

// ── QML 조회 함수 ────────────────────────────────────────────────────────────
QString DeviceModel::cameraId(const QString &ip) const {
  auto it = byIp_.find(ip);
  int idx = it != byIp_.end() ? *it : -1;
  return idx >= 0 ? devices_[idx].cameraId : "";
}

// getHistory removed as history tracking is now fully handled by getMetaHistory

QVariantList DeviceModel::getMetaHistory(const QString &cameraId,
                                         const QString &field) const {
  if (!cameraManager_)
    return {};
  auto *cam = cameraManager_->Get(cameraId.toStdString());
  if (!cam)
    return {};

  const auto &toList = [](const std::deque<float> &d) {
    QVariantList list;
    for (float v : d)
      list.append(static_cast<double>(v));
    return list;
  };

  if (field == "tmp")
    return toList(cam->MetaData_.tmp_);
  if (field == "tilt")
    return toList(cam->MetaData_.tilt_);
  if (field == "light")
    return toList(cam->MetaData_.light_);
  if (field == "hum")
    return toList(cam->MetaData_.hum_);
  return {};
}

// --- cameraId-based lookup methods ---

int DeviceModel::findIndexByCameraId(const QString &cameraId) const {
  for (int i = 0; i < devices_.size(); ++i) {
    if (devices_[i].cameraId == cameraId)
      return i;
  }
  return -1;
}

QString DeviceModel::deviceIp(const QString &cameraId) const {
  int idx = findIndexByCameraId(cameraId);
  return idx >= 0 ? devices_[idx].ip : QString();
}

bool DeviceModel::hasMotor(const QString &cameraId) const {
  if (!cameraManager_)
    return false;
  auto *cam = cameraManager_->Get(cameraId.toStdString());
  return cam ? cam->Status_.motor_auto : false;
}

bool DeviceModel::hasIr(const QString &cameraId) const {
  if (!cameraManager_)
    return false;
  auto *cam = cameraManager_->Get(cameraId.toStdString());
  return cam ? cam->Status_.ir_on : false;
}

bool DeviceModel::hasHeater(const QString &cameraId) const {
  if (!cameraManager_)
    return false;
  auto *cam = cameraManager_->Get(cameraId.toStdString());
  return cam ? cam->Status_.heater_on : false;
}

bool DeviceModel::hasDeviceByCameraId(const QString &cameraId) const {
  return findIndexByCameraId(cameraId) >= 0;
}

void DeviceModel::clearAll() {
  beginResetModel();
  devices_.clear();
  byIp_.clear();
  endResetModel();
}

/**
 * @section Workflow Guide
 *
 * **[DeviceModel 데이터 수집 및 가공 가이드]**
 *
 * 1. 데이터 소스 바인딩 (Domain Linkage):
 *    - 본 모델은 도메인 레이어의 `CameraManager`로부터 하드웨어 상태 정보를
 * 수령합니다.
 *    - `refreshFromCameraManager` 내부에서 `std::deque` 기반의 메타데이터를 Qt
 * 친화적인 `QList` 및 `QVariantList`로 변환하여 QML 시각화 엔진에 최적화된
 * 형태로 공급합니다.
 *
 * 2. 기기별 고유 기능 식별:
 *    - `hasIr`, `hasHeater` 등은 카메라의 하드웨어 스펙 정보를 나타내며, 이는
 * QML 대시보드의 상태 아이콘 활성화 여부를 결정하는 주요 논리값이 됩니다.
 *
 * 3. 효율적인 조회 체계:
 *    - `byIp_` 해시 맵을 유지하여 대규모 장치 환경에서도 IP 주소 기반의 상태
 * 조회를 O(1) 수준으로 유지하도록 설계되었습니다.
 */
