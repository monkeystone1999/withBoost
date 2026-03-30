#pragma once
#include "../../Src/Domain/CameraManager.hpp"
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <vector>

/**
 * @file DeviceModel.hpp
 * @brief 물리 장치 상태 및 하드웨어 구성 관리 모델
 *
 * 이 파일은 네트워크에 연결된 각 하드웨어 장치(IP 단위)의 시스템 자원 정보(CPU,
 * Memory, Temp)와 가용 기능(Motor, IR, Heater)을 관리하는 모델을 정의합니다.
 */

/** @brief 모델 내부에서 관리하는 개별 장치 엔트리 */
struct DeviceEntry {
  QString ip;            /**< 장치 IP 주소 (고유 식별자) */
  QString cameraId;      /**< 해당 장치의 대표 카메라 ID */
  QString type;          /**< 장치 모델/타입 */
  bool isOnline = false; /**< 장치 생존 여부 */

  double cpu = 0.0;
  double memory = 0.0;
  double temp = 0.0;
  int uptime = 0;
  qint64 lastUpdate = 0;

  bool hasMotor = true;   /**< PTZ 모터 탑재 여부 */
  bool hasIr = false;     /**< IR 조명 탑재 여부 */
  bool hasHeater = false; /**< 히터 탑재 여부 */
};

/**
 * @class DeviceModel
 * @brief 하드웨어 자원 모니터링 및 기기 인벤토리 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 초기화 단계에서 setCameraManager()를 통해 데이터 소스를 연결합니다.
 * 2. refreshFromCameraManager()를 호출하여 도메인 레이어의 최신 장치 정보를 뷰
 * 모델로 동기화합니다.
 * 3. QML 레이어는 getMetaHistory()를 통해 온도/기울기 등의 시계열 데이터를
 * 획득하여 분석 그래프를 렌더링합니다.
 */
class DeviceModel : public QAbstractListModel {
  Q_OBJECT
public:
  /** @brief QML 접근을 위한 데이터 역할 정의 */
  enum Roles {
    IpRole = Qt::UserRole + 1, /**< IP 주소 */
    CameraIdRole,              /**< 관련 카메라 ID */
    TypeRole,                  /**< 기종 정보 */
    IsOnlineRole,              /**< 온라인 상태 */
    CpuRole,                   /**< CPU 부하 */
    MemoryRole,                /**< 메모리 부하 */
    TempRole,                  /**< 온도 */
    UptimeRole,                /**< 가동 시간 */
    HasMotorRole,              /**< 모터 유무 */
    HasIrRole,                 /**< IR 유무 */
    HasHeaterRole              /**< 히터 유무 */
  };

  explicit DeviceModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE QString cameraId(const QString &ip) const;

signals:
  void historyUpdated(const QString &ip);

public:
  /** @brief 도메인 레이어 Manager 주입 */
  void setCameraManager(CameraManager *mgr) { cameraManager_ = mgr; }
  /** @brief 관리 중인 장치 정보를 강제 갱신 */
  Q_INVOKABLE void refreshFromCameraManager();
  /**
   * @brief 카메라 메타데이터 시계열 이력 조회
   * @param field 조회 필드 (tmp, tilt, light, hum)
   */
  Q_INVOKABLE QVariantList getMetaHistory(const QString &cameraId,
                                          const QString &field) const;

  Q_INVOKABLE QString deviceIp(const QString &cameraId) const;
  Q_INVOKABLE bool hasMotor(const QString &cameraId) const;
  Q_INVOKABLE bool hasIr(const QString &cameraId) const;
  Q_INVOKABLE bool hasHeater(const QString &cameraId) const;
  Q_INVOKABLE bool hasDeviceByCameraId(const QString &cameraId) const;

public slots:
  /** @brief 모든 장치 목록 제거 */
  void clearAll();

private:
  int findIndexByCameraId(const QString &cameraId) const;

  CameraManager *cameraManager_ = nullptr;
  QList<DeviceEntry> devices_;
  QHash<QString, int> byIp_; // IP -> row index
};

/**
 * @section Workflow Guide
 *
 * **[DeviceModel 시스템 연동 워크플로우]**
 *
 * 1. 동기화: `refreshFromCameraManager` 실행 시 `CameraManager` 내부의
 * `CameraInfo` 객체군으로부터 IP 및 하드웨어 상태(`Status_`)를 추출하여 큐를
 * 재구성합니다.
 * 2. 텔레메트리 제공: 장치 세부 정보 화면(DeviceDetail.qml) 진입 시
 * `getHistory`를 통해 최근 20개의 자원 소모 스냅샷을 획득합니다.
 * 3. 기능 제어 판단: PTZ 조작 UI 노출 여부를 결정하기 위해 `hasMotor`
 * 프로퍼티를 참조하여 관련 컨트롤의 가시성(Visibility)을 제어합니다.
 */
