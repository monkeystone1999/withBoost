#pragma once
#include "../../Src/Domain/CameraManager.hpp"
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <vector>

struct DeviceSnapshot {
    qint64 timestamp = 0;
    double cpu = 0.0;
    double memory = 0.0;
    double temp = 0.0;
    int uptime = 0;
};

// ── DeviceEntry (Qt용 변환) ──────────────────────────────────────────────────
struct DeviceEntry {
  QString ip;
  QString cameraId;
  QString type;
  bool isOnline = false;

  double cpu = 0.0;
  double memory = 0.0;
  double temp = 0.0;
  int uptime = 0;
  qint64 lastUpdate = 0;

  bool hasMotor = true;
  bool hasIr = false;
  bool hasHeater = false;

  // History (최대 20개)
  QList<DeviceSnapshot> history;
};

class DeviceModel : public QAbstractListModel {
  Q_OBJECT
public:
  enum Roles {
    IpRole = Qt::UserRole + 1,
    CameraIdRole,
    TypeRole,
    IsOnlineRole,
    CpuRole,
    MemoryRole,
    TempRole,
    UptimeRole,
    HasMotorRole,
    HasIrRole,
    HasHeaterRole
  };

  explicit DeviceModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  // QML에서 ip로 디바이스 상태 조회
  Q_INVOKABLE int findIndexByIp(const QString &ip) const;
  Q_INVOKABLE bool hasDevice(const QString &ip) const;
  Q_INVOKABLE QString cameraId(const QString &ip) const;
  Q_INVOKABLE double cpu(const QString &ip) const;
  Q_INVOKABLE double memory(const QString &ip) const;
  Q_INVOKABLE double temp(const QString &ip) const;
  Q_INVOKABLE QVariantList getHistory(const QString &ip) const;

signals:
  void historyUpdated(const QString &ip);

public:
  void setCameraManager(CameraManager* mgr) { cameraManager_ = mgr; }
  Q_INVOKABLE void refreshFromCameraManager();
  Q_INVOKABLE QVariantList getMetaHistory(const QString& cameraId, const QString& field) const;

  Q_INVOKABLE QString deviceIp(const QString &cameraId) const;
  Q_INVOKABLE bool hasMotor(const QString &cameraId) const;
  Q_INVOKABLE bool hasIr(const QString &cameraId) const;
  Q_INVOKABLE bool hasHeater(const QString &cameraId) const;
  Q_INVOKABLE bool hasDeviceByCameraId(const QString &cameraId) const;

public slots:
  void clearAll();

private:
  int findIndexByCameraId(const QString &cameraId) const;

  CameraManager* cameraManager_ = nullptr;
  QList<DeviceEntry> devices_;
  QHash<QString, int> byIp_; // IP -> row index
};
