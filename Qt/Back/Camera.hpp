#pragma once
#include "../../Src/Domain/Camera.hpp"
#include <QAbstractListModel>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QRectF>
#include <QSet>
#include <QString>
#include <vector>

// ── §8 VIDEO_STREAMING_SPEC: Virtual ID–Based Card Model ─────────────────────
// CameraEntry now carries:
//   slotId     — permanent grid position identity (never reused per session)
//   splitCount — 1=normal, 2/3/4=tile split
//   cropRect   — normalised UV [0..1] crop for GPU tile rendering
// ─────────────────────────────────────────────────────────────────────────────

struct SlotInfo {
  int slotId;
  QString rtspUrl;
};
Q_DECLARE_METATYPE(SlotInfo)
Q_DECLARE_METATYPE(QList<SlotInfo>)

struct CameraEntry {
  int slotId = -1;
  QString title;
  QString rtspUrl;
  bool isOnline = false;
  QString cameraType;
  int splitCount = 1; // 1 = no split
  SplitDirection splitDirection = SplitDirection::None;
  QRectF cropRect = {0, 0, 1, 1}; // default: full frame
  int splitGroupId = -1;          // -1 when not split
  int splitIndex = 0;             // tile index within split group
  QString description;            // Kept for metadata
  int width = 320;                // Default width
  int height = 240;               // Default height
};

class CameraModel : public QAbstractListModel {
  Q_OBJECT
public:
  enum Roles {
    SlotIdRole = Qt::UserRole + 1,
    TitleRole,
    RtspUrlRole,
    IsOnlineRole,
    DescriptionRole,
    CardWidthRole,
    CardHeightRole,
    CameraTypeRole,
    SplitCountRole,
    CropRectRole,
    SplitDirectionRole
  };

  explicit CameraModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void swapSlots(int indexA, int indexB); // full entry swap
  Q_INVOKABLE void setOnline(int index, bool online);
  Q_INVOKABLE void splitSlot(int rowIndex, int tileCount, int direction = 0);
  Q_INVOKABLE void mergeSlots(int anyTileRowIndex, int tileCount);
  Q_INVOKABLE bool isSlotSplit(int slotId) const;
  Q_INVOKABLE bool applyAutoSplitForSlot(int slotId);
  Q_INVOKABLE void clearAll();

  Q_INVOKABLE int rowForSlot(int slotId) const;
  Q_INVOKABLE bool hasSlot(int slotId) const;
  Q_INVOKABLE QString titleForSlot(int slotId) const;
  Q_INVOKABLE QString rtspUrlForSlot(int slotId) const;
  Q_INVOKABLE bool isOnlineForSlot(int slotId) const;
  Q_INVOKABLE QRectF cropRectForSlot(int slotId) const;

  // Legacy alias kept for QML compatibility during transition
  Q_INVOKABLE void swapCameraUrls(int indexA, int indexB) {
    swapSlots(indexA, indexB);
  }

public slots:
  // New path: called by Core on GUI thread with pre-parsed snapshot
  void onStoreUpdated(std::vector<CameraData> snapshot);

signals:
  // Emits the new SlotInfo list for VideoManager::registerSlots
  void slotsUpdated(QList<SlotInfo> slots);
  // Legacy URL-only signal (VideoManager::registerUrls)
  void urlsUpdated(QStringList urls);
  // Fired when an existing camera comes back online (offline → online
  // transition)
  void cameraOnline(const QString &rtspUrl);

private:
  static QRectF computeCropRect(int tileIndex, int tileCount,
                                SplitDirection direction);
  void _emitSlotsUpdated();
  int _indexBySlotId(int slotId) const;
  bool _splitSlotAtIndex(int rowIndex, int tileCount, SplitDirection direction,
                         bool autoSplit);
  bool _mergeGroupByIndex(int anyTileRowIndex);
  bool _autoSplitForIndex(int rowIndex);

  int nextSlotId_ = 0;
  int nextSplitGroupId_ = 1;

  QList<CameraEntry> cameras_;
  QSet<int> autoSplitAppliedSlotIds_;
};
