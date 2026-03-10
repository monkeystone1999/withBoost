#include "CameraModel.hpp"
#include "../../Src/Config.hpp"
#include <QHash>
#include <QString>
#include <utility>

CameraModel::CameraModel(QObject *parent) : QAbstractListModel(parent) {}

int CameraModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_cameras.size();
}

QVariant CameraModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_cameras.size())
    return {};
  const auto &cam = m_cameras[index.row()];
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
  default:
    return {};
  }
}

bool CameraModel::setData(const QModelIndex &index, const QVariant &value,
                          int role) {
  if (!index.isValid() || index.row() >= m_cameras.size())
    return false;
  auto &cam = m_cameras[index.row()];
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
  return {
      {SlotIdRole, "slotId"},           {TitleRole, "title"},
      {RtspUrlRole, "rtspUrl"},         {IsOnlineRole, "isOnline"},
      {DescriptionRole, "description"}, {CardWidthRole, "cardWidth"},
      {CardHeightRole, "cardHeight"},   {CameraTypeRole, "cameraType"},
      {SplitCountRole, "splitCount"},   {CropRectRole, "cropRect"},
  };
}

void CameraModel::swapSlots(int indexA, int indexB) {
  if (indexA < 0 || indexB < 0 || indexA >= m_cameras.size() ||
      indexB >= m_cameras.size() || indexA == indexB)
    return;

  // slotId/cropRect/splitCount/splitGroupId/splitIndex 는 "그리드 위치"의
  // 속성이므로 그 자리에 유지한다. rtspUrl, title, cameraType, isOnline,
  // description 만 교환한다.
  auto &a = m_cameras[indexA];
  auto &b = m_cameras[indexB];
  std::swap(a.rtspUrl, b.rtspUrl);
  std::swap(a.title, b.title);
  std::swap(a.cameraType, b.cameraType);
  std::swap(a.isOnline, b.isOnline);
  std::swap(a.description, b.description);

  const auto idxA = createIndex(indexA, 0);
  const auto idxB = createIndex(indexB, 0);
  const QList<int> roles = {TitleRole, RtspUrlRole, IsOnlineRole,
                            DescriptionRole, CameraTypeRole};
  emit dataChanged(idxA, idxA, roles);
  emit dataChanged(idxB, idxB, roles);
}

void CameraModel::setOnline(int index, bool online) {
  if (index < 0 || index >= m_cameras.size())
    return;
  m_cameras[index].isOnline = online;
  auto idx = createIndex(index, 0);
  emit dataChanged(idx, idx, {IsOnlineRole});
}

QRectF CameraModel::computeCropRect(int tileIndex, int tileCount) {
  switch (tileCount) {
  case 2:
    return tileIndex == 0 ? QRectF(0.0, 0.0, 0.5, 1.0)
                          : QRectF(0.5, 0.0, 0.5, 1.0);
  case 3:
    if (tileIndex == 0)
      return QRectF(0.000, 0.0, 0.333, 1.0);
    if (tileIndex == 1)
      return QRectF(0.333, 0.0, 0.334, 1.0);
    return QRectF(0.667, 0.0, 0.333, 1.0);
  case 4:
    if (tileIndex == 0)
      return QRectF(0.0, 0.0, 0.5, 0.5);
    if (tileIndex == 1)
      return QRectF(0.5, 0.0, 0.5, 0.5);
    if (tileIndex == 2)
      return QRectF(0.0, 0.5, 0.5, 0.5);
    return QRectF(0.5, 0.5, 0.5, 0.5);
  default:
    return {0, 0, 1, 1};
  }
}

int CameraModel::_indexBySlotId(int slotId) const {
  for (int i = 0; i < m_cameras.size(); ++i) {
    if (m_cameras[i].slotId == slotId)
      return i;
  }
  return -1;
}

bool CameraModel::_isValidTileCount(int tileCount) const {
  return tileCount >= 2 && tileCount <= 4;
}

bool CameraModel::_splitSlotAtIndex(int rowIndex, int tileCount,
                                    bool autoSplit) {
  if (rowIndex < 0 || rowIndex >= m_cameras.size())
    return false;
  if (!_isValidTileCount(tileCount))
    return false;
  if (m_cameras[rowIndex].splitCount != 1)
    return false;
  if (!m_cameras[rowIndex].isOnline)
    return false;

  const CameraEntry original = m_cameras[rowIndex];
  const int splitGroupId = m_nextSplitGroupId++;

  beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
  m_cameras.removeAt(rowIndex);
  endRemoveRows();

  beginInsertRows(QModelIndex(), rowIndex, rowIndex + tileCount - 1);
  for (int i = 0; i < tileCount; ++i) {
    CameraEntry e;
    e.slotId = m_nextSlotId++;
    e.rtspUrl = original.rtspUrl;
    e.title = original.title + " [" + QString::number(i + 1) + "/" +
              QString::number(tileCount) + "]";
    e.isOnline = original.isOnline;
    e.cameraType = original.cameraType;
    e.splitCount = tileCount;
    e.cropRect = computeCropRect(i, tileCount);
    e.splitGroupId = splitGroupId;
    e.splitIndex = i;
    m_cameras.insert(rowIndex + i, e);
    if (autoSplit)
      m_autoSplitAppliedSlotIds.insert(e.slotId);
  }
  endInsertRows();

  m_autoSplitAppliedSlotIds.remove(original.slotId);
  _emitSlotsUpdated();
  return true;
}

bool CameraModel::_mergeGroupByIndex(int anyTileRowIndex) {
  if (anyTileRowIndex < 0 || anyTileRowIndex >= m_cameras.size())
    return false;

  const CameraEntry base = m_cameras[anyTileRowIndex];
  if (base.splitCount <= 1 || base.splitGroupId < 0)
    return false;

  // No reset model — use granular list updates
  QList<int> groupIndices;
  groupIndices.reserve(base.splitCount);
  for (int i = 0; i < m_cameras.size(); ++i) {
    if (m_cameras[i].splitGroupId == base.splitGroupId)
      groupIndices.push_back(i);
  }

  if (groupIndices.size() < 2)
    return false;

  int firstIndex = groupIndices.front();
  QString mergedTitle = base.title;
  const int marker = mergedTitle.lastIndexOf(" [");
  if (marker > 0 && mergedTitle.endsWith(']'))
    mergedTitle = mergedTitle.left(marker);

  // 1. Remove all tiles in the group
  beginRemoveRows(QModelIndex(), groupIndices.front(), groupIndices.back());
  for (int i = groupIndices.size() - 1; i >= 0; --i) {
    const int idx = groupIndices[i];
    m_autoSplitAppliedSlotIds.remove(m_cameras[idx].slotId);
    m_cameras.removeAt(idx);
  }
  endRemoveRows();

  // 2. Insert the single merged entry
  CameraEntry merged;
  merged.slotId = m_nextSlotId++;
  merged.rtspUrl = base.rtspUrl;
  merged.title = mergedTitle;
  merged.isOnline = base.isOnline;
  merged.cameraType = base.cameraType;
  merged.splitCount = 1;
  merged.cropRect = {0, 0, 1, 1};
  merged.splitGroupId = -1;
  merged.splitIndex = 0;

  beginInsertRows(QModelIndex(), firstIndex, firstIndex);
  m_cameras.insert(firstIndex, merged);
  endInsertRows();

  _emitSlotsUpdated();
  return true;
}

bool CameraModel::_autoSplitForIndex(int rowIndex) {
  if (rowIndex < 0 || rowIndex >= m_cameras.size())
    return false;
  const auto &entry = m_cameras[rowIndex];
  if (!entry.isOnline || entry.splitCount != 1)
    return false;
  if (m_autoSplitAppliedSlotIds.contains(entry.slotId))
    return false;

  const std::string url = entry.rtspUrl.toStdString();
  const int tileCount = Config::autoSplitTileCountForUrl(url);
  if (!_isValidTileCount(tileCount))
    return false;

  return _splitSlotAtIndex(rowIndex, tileCount, true);
}

void CameraModel::splitSlot(int rowIndex, int tileCount) {
  _splitSlotAtIndex(rowIndex, tileCount, false);
}

void CameraModel::mergeSlots(int anyTileRowIndex, int) {
  _mergeGroupByIndex(anyTileRowIndex);
}

bool CameraModel::isSlotSplit(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  return idx >= 0 && m_cameras[idx].splitCount > 1;
}

bool CameraModel::applyAutoSplitForSlot(int slotId) {
  const int idx = _indexBySlotId(slotId);
  return _autoSplitForIndex(idx);
}

void CameraModel::clearAll() {
  beginResetModel();
  m_cameras.clear();
  m_autoSplitAppliedSlotIds.clear();
  m_nextSlotId = 0;
  m_nextSplitGroupId = 1;
  endResetModel();
  _emitSlotsUpdated();
}

bool CameraModel::hasSlot(int slotId) const {
  return _indexBySlotId(slotId) >= 0;
}

QString CameraModel::titleForSlot(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  if (idx < 0)
    return QString();
  return m_cameras[idx].title;
}

QString CameraModel::rtspUrlForSlot(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  if (idx < 0)
    return QString();
  return m_cameras[idx].rtspUrl;
}

bool CameraModel::isOnlineForSlot(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  if (idx < 0)
    return false;
  return m_cameras[idx].isOnline;
}

QRectF CameraModel::cropRectForSlot(int slotId) const {
  const int idx = _indexBySlotId(slotId);
  if (idx < 0)
    return QRectF(0, 0, 1, 1);
  return m_cameras[idx].cropRect;
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
  for (int i = 0; i < m_cameras.size(); ++i) {
    auto &entry = m_cameras[i];
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
    e.slotId = m_nextSlotId++;
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
    beginInsertRows(QModelIndex(), m_cameras.size(),
                    m_cameras.size() + newEntries.size() - 1);
    for (const auto &e : newEntries)
      m_cameras.append(e);
    endInsertRows();
  }

  _emitSlotsUpdated();

  QList<int> candidateSlots;
  candidateSlots.reserve(m_cameras.size());
  for (const auto &entry : m_cameras) {
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
  slotList.reserve(m_cameras.size());
  for (const auto &cam : m_cameras) {
    slotList.append({cam.slotId, cam.rtspUrl});
    if (!cam.rtspUrl.isEmpty() && !seenUrls.contains(cam.rtspUrl)) {
      seenUrls.insert(cam.rtspUrl);
      urls << cam.rtspUrl;
    }
  }
  emit slotsUpdated(slotList);
  emit urlsUpdated(urls);
}
