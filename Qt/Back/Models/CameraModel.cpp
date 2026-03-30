#include "CameraModel.hpp"
#include "../../Src/Config.hpp"
#include <QHash>
#include <QString>
#include <utility>

CameraModel::CameraModel(QObject *parent) : QAbstractListModel(parent) {}

int CameraModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return cameras_.size();
}

QVariant CameraModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= cameras_.size())
    return {};
  const auto &cam = cameras_[index.row()];
  switch (role) {
  case SlotIdRole:
    return cam.slotId;
  case TitleRole:
    return cam.title;
  case IsOnlineRole:
    return cam.isOnline;
  case DescriptionRole:
    return cam.description;
  case CardWidthRole:
    return cam.width;
  case CardHeightRole:
    return cam.height;
  case CameraTypeRole:
    return cam.cameraType;
  case SplitCountRole:
    return cam.splitCount;
  case CropRectRole:
    return cam.cropRect;
  case SplitDirectionRole:
    return static_cast<int>(cam.splitDirection);
  case CameraIdRole:
    return cam.cameraId;
  default:
    return QVariant();
  }
}

bool CameraModel::setData(const QModelIndex &index, const QVariant &value,
                          int role) {
  if (!index.isValid() || index.row() >= cameras_.size())
    return false;
  auto &cam = cameras_[index.row()];
  switch (role) {
  case TitleRole:
    cam.title = value.toString();
    break;
  case IsOnlineRole:
    cam.isOnline = value.toBool();
    break;
  case CameraTypeRole:
    cam.cameraType = value.toString();
    break;
  default:
    return false;
  }
  emit dataChanged(index, index, {role});
  return true;
}

QHash<int, QByteArray> CameraModel::roleNames() const {
  return {
      {SlotIdRole, "slotId"},         {TitleRole, "title"},
      {IsOnlineRole, "isOnline"},     {DescriptionRole, "description"},
      {CardWidthRole, "cardWidth"},   {CardHeightRole, "cardHeight"},
      {CameraTypeRole, "cameraType"}, {SplitCountRole, "splitCount"},
      {CropRectRole, "cropRect"},     {SplitDirectionRole, "splitDirection"},
      {CameraIdRole, "cameraId"}};
}

void CameraModel::swapSlots(int indexA, int indexB) {
  if (indexA < 0 || indexB < 0 || indexA >= cameras_.size() ||
      indexB >= cameras_.size() || indexA == indexB)
    return;

  auto &a = cameras_[indexA];
  auto &b = cameras_[indexB];

  // ── slotId 만 유지, 나머지 모든 content 필드를 교환 ─────────────────────
  // (Detached CameraWindow 는 sourceSlotId 를 추적하므로, slotId 가 이동하면
  //  CameraWindow 와 CameraCard 사이의 동기화가 끊어집니다.)
  std::swap(a.title, b.title);
  std::swap(a.cameraId, b.cameraId);
  std::swap(a.isOnline, b.isOnline);
  std::swap(a.cameraType, b.cameraType);
  std::swap(a.description, b.description);
  std::swap(a.splitCount, b.splitCount);
  std::swap(a.splitDirection, b.splitDirection);
  std::swap(a.cropRect, b.cropRect);
  std::swap(a.splitGroupId, b.splitGroupId);
  std::swap(a.splitIndex, b.splitIndex);
  std::swap(a.width, b.width);
  std::swap(a.height, b.height);

  const QList<int> allRoles = {
      TitleRole,          IsOnlineRole, DescriptionRole,
      CameraTypeRole,     CropRectRole, SplitCountRole,
      SplitDirectionRole, CameraIdRole}; // [CRITICAL] CameraIdRole emits QML
                                         // stream change

  _emitSlotsUpdated();
  emit dataChanged(createIndex(indexA, 0), createIndex(indexA, 0), allRoles);
  emit dataChanged(createIndex(indexB, 0), createIndex(indexB, 0), allRoles);
}

void CameraModel::setOnline(int index, bool online) {
  if (index < 0 || index >= cameras_.size())
    return;
  cameras_[index].isOnline = online;
  auto idx = createIndex(index, 0);
  emit dataChanged(idx, idx, {IsOnlineRole});
}

QRectF CameraModel::computeCropRect(int tileIndex, int tileCount,
                                    SplitDirection direction) {
  // -- Col: X-axis n-way split -----------------------------------------------
  if (direction == SplitDirection::Col) {
    if (tileCount < 2 || tileCount > 4)
      return {0, 0, 1, 1};
    const qreal w = 1.0 / tileCount;
    // Calculate with precision to avoid gaps
    const qreal x = tileIndex * w;
    const qreal end = (tileIndex == tileCount - 1) ? 1.0 : x + w;
    return QRectF(x, 0.0, end - x, 1.0);
  }

  // -- Row: Y-axis n-way split -----------------------------------------------
  if (direction == SplitDirection::Row) {
    if (tileCount < 2 || tileCount > 3)
      return {0, 0, 1, 1};
    const qreal h = 1.0 / tileCount;
    const qreal y = tileIndex * h;
    const qreal end = (tileIndex == tileCount - 1) ? 1.0 : y + h;
    return QRectF(0.0, y, 1.0, end - y);
  }

  // -- Grid: 2x2 Grid (tileCount == 4 only) ----------------------------------
  if (direction == SplitDirection::Grid && tileCount == 4) {
    const qreal col = (tileIndex % 2) * 0.5;
    const qreal row = (tileIndex / 2) * 0.5;
    return QRectF(col, row, 0.5, 0.5);
  }

  return {0, 0, 1, 1};
}

int CameraModel::_indexBySlotId(int slotId) const {
  for (int i = 0; i < cameras_.size(); ++i) {
    if (cameras_[i].slotId == slotId)
      return i;
  }
  return -1;
}

void CameraModel::_emitSlotsUpdated() {
  QList<SlotInfo> SlotList;
  for (const auto &c : cameras_) {
    SlotInfo s;
    s.slotId = c.slotId;
    s.cameraId = c.cameraId;
    SlotList.append(s);
  }
  emit slotsUpdated(SlotList);

  QStringList cids;
  for (const auto &c : cameras_) {
    if (!cids.contains(c.cameraId))
      cids.append(c.cameraId);
  }
  emit cameraIdsUpdated(cids);
}

bool CameraModel::_splitSlotAtIndex(int rowIndex, int tileCount,
                                    SplitDirection direction, bool autoSplit) {
  if (rowIndex < 0 || rowIndex >= cameras_.size())
    return false;

  bool validCount = false;
  if (direction == SplitDirection::Col)
    validCount = (tileCount >= 2 && tileCount <= 4);
  if (direction == SplitDirection::Row)
    validCount = (tileCount == 2 || tileCount == 3);
  if (direction == SplitDirection::Grid)
    validCount = (tileCount == 4);

  if (!validCount)
    return false;
  if (cameras_[rowIndex].splitCount != 1)
    return false;
  if (!cameras_[rowIndex].isOnline)
    return false;

  const CameraEntry original = cameras_[rowIndex];
  const int splitGroupId = nextSplitGroupId_++;

  beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
  cameras_.removeAt(rowIndex);
  endRemoveRows();

  beginInsertRows(QModelIndex(), rowIndex, rowIndex + tileCount - 1);
  for (int i = 0; i < tileCount; ++i) {
    CameraEntry e;
    e.slotId = nextSlotId_++;
    e.cameraId = original.cameraId;
    e.title = original.title + " [" + QString::number(i + 1) + "/" +
              QString::number(tileCount) + "]";
    e.isOnline = original.isOnline;
    e.cameraType = original.cameraType;
    e.splitCount = tileCount;
    e.splitDirection = direction;
    e.splitGroupId = splitGroupId;
    e.splitIndex = i;
    e.cropRect = computeCropRect(i, tileCount, direction);
    e.description = original.description;
    cameras_.insert(rowIndex + i, e);
    if (autoSplit)
      autoSplitAppliedSlotIds_.insert(e.slotId);
  }
  endInsertRows();

  autoSplitAppliedSlotIds_.remove(original.slotId);
  _emitSlotsUpdated();
  return true;
}

bool CameraModel::_mergeGroupByIndex(int anyTileRowIndex) {
  if (anyTileRowIndex < 0 || anyTileRowIndex >= cameras_.size())
    return false;

  const CameraEntry target = cameras_[anyTileRowIndex];
  if (target.splitCount <= 1 || target.splitGroupId < 0)
    return false;

  QList<int> groupIndices;
  for (int i = 0; i < cameras_.size(); ++i) {
    if (cameras_[i].splitGroupId == target.splitGroupId)
      groupIndices.push_back(i);
  }

  if (groupIndices.size() < 2)
    return false;

  // We will keep the first tile in the list as the "merged" card's position
  int firstPos = groupIndices.front();

  // 1. Remove all tiles EXCEPT the first one
  // Must remove in reverse order to keep indices valid during removal
  for (int i = groupIndices.size() - 1; i > 0; --i) {
    int posToRemove = groupIndices[i];
    beginRemoveRows(QModelIndex(), posToRemove, posToRemove);
    autoSplitAppliedSlotIds_.remove(cameras_[posToRemove].slotId);
    cameras_.removeAt(posToRemove);
    endRemoveRows();

    // If the removed tile was before any other tiles in our list (shouldn't
    // happen with sorted groupIndices), we'd need to adjust. But groupIndices
    // is sorted (0..size).
  }

  // 2. Update the first tile in-place
  auto &merged = cameras_[firstPos];
  // Preserve merged.slotId -> Identity preserved!

  // Cleanup title
  QString newTitle = merged.title;
  const int marker = newTitle.lastIndexOf(" [");
  if (marker > 0 && newTitle.endsWith(']'))
    newTitle = newTitle.left(marker);

  merged.title = newTitle;
  merged.splitCount = 1;
  merged.cropRect = {0, 0, 1, 1};
  merged.splitGroupId = -1;
  merged.splitIndex = 0;
  merged.splitDirection = SplitDirection::Col; // Reset to default

  emit dataChanged(createIndex(firstPos, 0), createIndex(firstPos, 0));
  _emitSlotsUpdated();
  return true;
}

bool CameraModel::_autoSplitForIndex(int rowIndex) {
  if (rowIndex < 0 || rowIndex >= cameras_.size())
    return false;
  const auto &entry = cameras_[rowIndex];
  if (!entry.isOnline || entry.splitCount != 1)
    return false;
  if (autoSplitAppliedSlotIds_.contains(entry.slotId))
    return false;

  const std::string cid = entry.cameraId.toStdString();
  const int tileCount = Config::autoSplitTileCountForCameraId(cid);

  return _splitSlotAtIndex(rowIndex, tileCount, SplitDirection::Col, true);
}

void CameraModel::splitSlot(int rowIndex, int tileCount, int direction) {
  SplitDirection dir = SplitDirection::Col; // Default
  if (direction == 1)
    dir = SplitDirection::Row;
  if (direction == 2)
    dir = SplitDirection::Grid;
  _splitSlotAtIndex(rowIndex, tileCount, dir, false);
}

void CameraModel::mergeSlots(int anyTileRowIndex, int) {
  _mergeGroupByIndex(anyTileRowIndex);
}

bool CameraModel::isSlotSplit(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  return idx >= 0 && cameras_[idx].splitCount > 1;
}

bool CameraModel::applyAutoSplitForSlot(int slotId) {
  const int idx = _indexBySlotId(slotId);
  return _autoSplitForIndex(idx);
}

void CameraModel::clearAll() {
  beginResetModel();
  cameras_.clear();
  autoSplitAppliedSlotIds_.clear();
  nextSlotId_ = 0;
  nextSplitGroupId_ = 1;
  endResetModel();
  _emitSlotsUpdated();
}

int CameraModel::rowForSlot(int slotId) const { return _indexBySlotId(slotId); }

bool CameraModel::hasSlot(int slotId) const {
  return _indexBySlotId(slotId) >= 0;
}

QString CameraModel::titleForSlot(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  if (idx < 0)
    return QString();
  return cameras_[idx].title;
}

bool CameraModel::isOnlineForSlot(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  if (idx < 0)
    return false;
  return cameras_[idx].isOnline;
}

QRectF CameraModel::cropRectForSlot(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  if (idx < 0)
    return QRectF(0, 0, 1, 1);
  return cameras_[idx].cropRect;
}

QString CameraModel::cameraIdForSlot(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  if (idx < 0)
    return QString();
  return cameras_[idx].cameraId;
}

void CameraModel::refreshFromCameraManager() {
  if (!cameraManager_)
    return;

  QHash<QString, CameraInfo *> snapshotByCid;
  for (auto &pair : cameraManager_->getCameras()) {
    QString cid = QString::fromStdString(pair.second.ip_) + "/" +
                  QString::number(pair.second.index_);
    snapshotByCid.insert(cid, &pair.second);
  }

  QSet<QString> representedCids;

  // 1. Update existing entries
  for (int i = 0; i < cameras_.size(); ++i) {
    auto &entry = cameras_[i];
    auto it = snapshotByCid.find(entry.cameraId);

    bool changed = false;
    QList<int> changedRoles;

    if (it == snapshotByCid.end()) {
      // Stream gone from server -> Mark offline
      if (entry.isOnline) {
        entry.isOnline = false;
        changed = true;
        changedRoles << IsOnlineRole;
      }
    } else {
      CameraInfo *snap = it.value();
      representedCids.insert(entry.cameraId);
      bool snapOnline = true; // Assume online if it's in CameraManager

      if (entry.isOnline != snapOnline) {
        const bool wasOffline = !entry.isOnline;
        entry.isOnline = snapOnline;
        changed = true;
        changedRoles << IsOnlineRole;
        if (wasOffline && snapOnline) {
          emit cameraOnline(entry.cameraId);
        }
      }

      if (entry.splitCount == 1) {
        QString snapTitle = entry.cameraId; // Use ID as title since Src doesn't
                                            // store display titles
        if (entry.title != snapTitle) {
          entry.title = snapTitle;
          changed = true;
          changedRoles << TitleRole;
        }
      }
    }

    if (changed) {
      emit dataChanged(createIndex(i, 0), createIndex(i, 0), changedRoles);
    }
  }

  // 2. Insert absolute new streams as new rows
  QList<CameraEntry> newEntries;
  for (auto it = snapshotByCid.constBegin(); it != snapshotByCid.constEnd();
       ++it) {
    if (representedCids.contains(it.key()))
      continue;

    CameraEntry e;
    e.slotId = nextSlotId_++;
    e.title = it.key();
    e.cameraId = it.key();
    e.isOnline = true;
    e.cameraType = "Unknown";
    e.splitCount = 1;
    e.cropRect = {0, 0, 1, 1};
    e.splitGroupId = -1;
    e.splitIndex = 0;
    newEntries.append(e);
  }

  if (!newEntries.isEmpty()) {
    beginInsertRows(QModelIndex(), cameras_.size(),
                    cameras_.size() + newEntries.size() - 1);
    for (const auto &e : newEntries)
      cameras_.append(e);
    endInsertRows();
  }

  _emitSlotsUpdated();

  QList<int> candidateSlots;
  candidateSlots.reserve(cameras_.size());
  for (const auto &entry : cameras_) {
    if (entry.splitCount == 1 && entry.isOnline)
      candidateSlots.push_back(entry.slotId);
  }
  for (int slotId : candidateSlots)
    applyAutoSplitForSlot(slotId);
}

void CameraModel::refreshSensorInfo() {
  if (!cameraManager_)
    return;
  bool anyUpdated = false;

  for (int i = 0; i < cameras_.size(); ++i) {
    auto *info = cameraManager_->Get(cameras_[i].cameraId.toStdString());
    if (info && !info->MetaData_.tmp_.empty()) {
      cameras_[i].deviceInfo.hum = info->MetaData_.hum_.front();
      cameras_[i].deviceInfo.light = info->MetaData_.light_.front();
      cameras_[i].deviceInfo.tilt = info->MetaData_.tilt_.front();
      cameras_[i].deviceInfo.tmp = info->MetaData_.tmp_.front();
      anyUpdated = true;
      emit sensorDataUpdated(cameras_[i].cameraId);
    }
  }
}

QJsonObject CameraModel::sensorInfoForCameraId(const QString &cameraId) const {
  QJsonObject obj;
  for (const auto &entry : cameras_) {
    if (entry.cameraId == cameraId) {
      obj["hum"] = entry.deviceInfo.hum;
      obj["light"] = entry.deviceInfo.light;
      obj["tilt"] = entry.deviceInfo.tilt;
      obj["tmp"] = entry.deviceInfo.tmp;
      return obj;
    }
  }
  return obj;
}

/**
 * @section Workflow Guide
 *
 * **[CameraModel 핵심 도메인 로직 가이드]**
 *
 * 1. 슬롯 기반 위치 관리 (Virtual ID System):
 *    - 사용자가 그리드에서 항목을 드래그하여 순서를 바꿀 때(`swapSlots`), 내부
 * 데이터 엔트리는 교환되지만 `slotId`는 유지됩니다.
 *    - 이는 해당 카메라를 추적하는 외부 윈도우나 비디오 소켓이 위치 변경에
 * 영향을 받지 않도록 설계된 아키텍처적 장치입니다.
 *
 * 2. 동적 화면 분할 (Camera Splitting):
 *    - `beginRemoveRows` / `beginInsertRows` 매크로를 사용하여 QML 리스트 뷰가
 * 부드러운 애니메이션과 함께 타일 생성을 처리할 수 있도록 동기화합니다.
 *    - 분할 시 소스 카메라 주소(`cameraId`)는 동일하게 공유되지만, 각 엔트리는
 * 고유의 `CropRect`를 가져 셰이더 단계에서 다른 영역을 렌더링하게 됩니다.
 *
 * 3. 기기 센서 데이터 허브:
 *    - `updateSensorInfo`는 특정 카메라 ID를 가진 모든 슬롯(분할된 타일 포함)에
 * 대해 텔레메트리 값을 일괄 갱신하며, `sensorDataUpdated` 시그널 하나로
 * 최적화된 QML 바인딩 갱신을 수행합니다.
 */
