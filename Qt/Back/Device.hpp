#pragma once
#include "../../Src/Domain/Device.hpp"
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <vector>

// ── DeviceEntry (Qt용 변환) ──────────────────────────────────────────────────
struct DeviceEntry {
  QString ip;
  QString rtspUrl;
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
    RtspUrlRole,
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

  // QML 조회
  Q_INVOKABLE bool hasDevice(const QString &ip) const;
  Q_INVOKABLE QString rtspUrl(const QString &ip) const;
  Q_INVOKABLE double cpu(const QString &ip) const;
  Q_INVOKABLE double memory(const QString &ip) const;
  Q_INVOKABLE double temp(const QString &ip) const;

  // History lookup (max 20 entries)
  Q_INVOKABLE QVariantList getHistory(const QString &ip) const;

  // rtspUrl-based lookup (for QML calls with RTSP URLs)
  Q_INVOKABLE QString deviceIp(const QString &rtspUrl) const;
  Q_INVOKABLE bool hasMotor(const QString &rtspUrl) const;
  Q_INVOKABLE bool hasIr(const QString &rtspUrl) const;
  Q_INVOKABLE bool hasHeater(const QString &rtspUrl) const;
  Q_INVOKABLE bool hasDeviceByUrl(const QString &rtspUrl) const;

public slots:
  // Called by Core (GUI thread) with pre-parsed snapshot
  void onStoreUpdated(std::vector<DeviceIntegrated> snapshot);

private:
  int findIndexByIp(const QString &ip) const;
  int findIndexByRtspUrl(const QString &rtspUrl) const;

  QList<DeviceEntry> devices_;
  QHash<QString, int> byIp_; // IP -> row index
};
