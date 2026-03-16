#include "Camera.hpp"
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
  case RtspUrlRole:
    return cam.rtspUrl;
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
  case RtspUrlRole:
    cam.rtspUrl = value.toString();
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
  return {{SlotIdRole, "slotId"},
          {TitleRole, "title"},
          {RtspUrlRole, "rtspUrl"},
          {IsOnlineRole, "isOnline"},
          {DescriptionRole, "description"},
          {CardWidthRole, "cardWidth"},
          {CardHeightRole, "cardHeight"},
          {CameraTypeRole, "cameraType"},
          {SplitCountRole, "splitCount"},
          {CropRectRole, "cropRect"},
          {SplitDirectionRole, "splitDirection"}};
}

void CameraModel::swapSlots(int indexA, int indexB) {
  if (indexA < 0 || indexB < 0 || indexA >= cameras_.size() ||
      indexB >= cameras_.size() || indexA == indexB)
    return;

  auto &a = cameras_[indexA];
  auto &b = cameras_[indexB];

  // ── slotId 만 유지, 나머지 모든 content 필드를 교환 ─────────────────────
  // 규칙: 각 Card는 Crop, CameraId(rtspUrl), Group 3가지를 가지며
  //       Swap 시 이 3가지 전부를 교환함. Split ↔ 일반 swap 시 split 정보도
  //       교환되어 상대 카드가 Split이 되고 기존 Split 카드는 일반이 됨.
  std::swap(a.title, b.title);
  std::swap(a.rtspUrl, b.rtspUrl);
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

  // splitGroupId 가 교환되었으므로 그룹 구조가 자동으로 재편성됨:
  //   - slotId 는 유지 → CameraWindow 는 여전히 올바른 slot 을 팔로우
  //   - 시블링 타일은 splitGroupId 로 멤버십을 판단하므로 별도 전파 불필요

  const QList<int> allRoles = {TitleRole,       RtspUrlRole,       IsOnlineRole,
                               DescriptionRole, CameraTypeRole,    CropRectRole,
                               SplitCountRole,  SplitDirectionRole};

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
    e.rtspUrl = original.rtspUrl;
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

  const std::string url = entry.rtspUrl.toStdString();
  const int tileCount = Config::autoSplitTileCountForUrl(url);

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

QString CameraModel::rtspUrlForSlot(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  if (idx < 0)
    return QString();
  return cameras_[idx].rtspUrl;
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

void CameraModel::onStoreUpdated(std::vector<CameraData> snapshot) {
  QHash<QString, CameraData> snapshotByUrl;
  for (const auto &cam : snapshot) {
    const QString url = QString::fromStdString(cam.rtspUrl);
    if (url.isEmpty())
      continue;
    snapshotByUrl.insert(url, cam);
  }

  QSet<QString> representedUrls;

  // 1. Update existing entries
  for (int i = 0; i < cameras_.size(); ++i) {
    auto &entry = cameras_[i];
    auto it = snapshotByUrl.find(entry.rtspUrl);

    bool changed = false;
    QList<int> changedRoles;

    if (it == snapshotByUrl.end()) {
      // Stream gone from server -> Mark offline
      if (entry.isOnline) {
        entry.isOnline = false;
        changed = true;
        changedRoles << IsOnlineRole;
      }
    } else {
      const CameraData &snap = it.value();
      representedUrls.insert(entry.rtspUrl);

      if (entry.isOnline != snap.isOnline) {
        const bool wasOffline = !entry.isOnline;
        entry.isOnline = snap.isOnline;
        changed = true;
        changedRoles << IsOnlineRole;
        // Emit reconnection signal when coming back online
        if (wasOffline && snap.isOnline) {
          emit cameraOnline(entry.rtspUrl);
        }
      }

      QString snapType = QString::fromStdString(snap.cameraType);
      if (entry.cameraType != snapType) {
        entry.cameraType = snapType;
        changed = true;
        changedRoles << CameraTypeRole;
      }

      if (entry.splitCount == 1) {
        QString snapTitle = QString::fromStdString(snap.title);
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
  for (auto it = snapshotByUrl.constBegin(); it != snapshotByUrl.constEnd();
       ++it) {
    if (representedUrls.contains(it.key()))
      continue;

    const CameraData &snap = it.value();
    CameraEntry e;
    e.slotId = nextSlotId_++;
    e.title = QString::fromStdString(snap.title);
    e.rtspUrl = it.key();
    e.isOnline = snap.isOnline;
    e.cameraType = QString::fromStdString(snap.cameraType);
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

void CameraModel::_emitSlotsUpdated() {
  QList<SlotInfo> slotList;
  QStringList urls;
  QSet<QString> seenUrls;
  slotList.reserve(cameras_.size());
  for (const auto &cam : cameras_) {
    slotList.append({cam.slotId, cam.rtspUrl});
    if (!cam.rtspUrl.isEmpty() && !seenUrls.contains(cam.rtspUrl)) {
      seenUrls.insert(cam.rtspUrl);
      urls << cam.rtspUrl;
    }
  }
  emit slotsUpdated(slotList);
  emit urlsUpdated(urls);
}
