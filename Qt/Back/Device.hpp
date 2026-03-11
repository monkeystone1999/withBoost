#pragma once
#include "../../Src/Domain/Device.hpp"
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <vector>

// rtspUrl 하나에 연결된 실제 기기의 제어 능력
struct DeviceCapability {
  bool motor = true; // 방향 4키 — 기본 제공
  bool ir = false;
  bool heater = false;
};

struct DeviceEntry {
  QString rtspUrl;
  QString deviceIp; // 서버가 알려준 제어 대상 IP
  QString title;
  bool isOnline = false;
  DeviceCapability cap;
};

class DeviceModel : public QAbstractListModel {
  Q_OBJECT
public:
  enum Roles {
    RtspUrlRole = Qt::UserRole + 1,
    DeviceIpRole,
    TitleRole,
    IsOnlineRole,
    HasMotorRole,
    HasIrRole,
    HasHeaterRole
  };

  explicit DeviceModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  // QML / CameraCard 에서 rtspUrl 로 기능 조회
  Q_INVOKABLE bool hasDevice(const QString &rtspUrl) const;
  Q_INVOKABLE bool hasMotor(const QString &rtspUrl) const;
  Q_INVOKABLE bool hasIr(const QString &rtspUrl) const;
  Q_INVOKABLE bool hasHeater(const QString &rtspUrl) const;
  Q_INVOKABLE QString deviceIp(const QString &rtspUrl) const;

public slots:
  // Called by Core (GUI thread) with pre-parsed snapshot — no JSON
  void onStoreUpdated(std::vector<DeviceData> snapshot);

private:
  int findIndexByRtspUrl(const QString &rtspUrl) const;

  QList<DeviceEntry> m_devices;
  QHash<QString, int> m_byUrl; // rtspUrl -> row index (빠른 조회)
};
