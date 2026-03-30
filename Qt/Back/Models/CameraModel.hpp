

#pragma once

#include "../../../Src/Domain/CameraManager.hpp"

#include <QAbstractListModel>
#include <QJsonObject>
#include <QRectF>
#include <QSet>
#include <QString>
#include <QVariant>

/** @brief 카메라 카드의 화면 분할 방향 정의 */
enum class SplitDirection {
  None = 0, /**< 분할 없음 */
  Col,      /**< 수직 열 분할 (가로로 나열) */
  Row,      /**< 수평 행 분할 (세로로 나열) */
  Grid      /**< 2x2 격자 분할 */
};

/** @brief 장치에서 수신된 환경 센서 데이터 세트 */
struct DeviceInfo {
  double hum = 0.0;   /**< 습도 (%) */
  double light = 0.0; /**< 조도 (Lux) */
  double tilt = 0.0;  /**< 기울기 (Degree) */
  double tmp = 0.0;   /**< 온도 (Celsius) */
};

/** @brief 비디오 렌더러와 동기화하기 위한 슬롯-카메라 매핑 정보 */
struct SlotInfo {
  int slotId;       /**< UI 그리드 내 고유 슬롯 번호 */
  QString cameraId; /**< 해당 슬롯에 할당된 카메라 ID */
};
Q_DECLARE_METATYPE(SlotInfo)
Q_DECLARE_METATYPE(QList<SlotInfo>)

/** @brief 모델 내부에서 관리하는 개별 카메라/슬롯 엔트리 */
struct CameraEntry {
  int slotId = -1;       /**< 슬롯 고유 아이디 (세션 유지) */
  QString title;         /**< QML에 표시될 제목 */
  QString cameraId;      /**< 소스 카메라 식별자 */
  bool isOnline = false; /**< 온라인 상태 */
  QString cameraType;    /**< 장치 종류 (Fixed/PTZ 등) */
  int splitCount = 1;    /**< 현재 슬롯의 분할 개수 (1=단일) */
  SplitDirection splitDirection = SplitDirection::None; /**< 분할 방식 */
  QRectF cropRect = {0, 0, 1,
                     1}; /**< 전체 이미지 중 해당 타일이 차지하는 UV 영역 */
  int splitGroupId = -1; /**< 동일 카메라 분할 그룹 ID */
  int splitIndex = 0;    /**< 그룹 내 타일 순서 */
  QString description;   /**< 부가 설명 */
  int width = 320;       /**< UI 권장 너비 */
  int height = 240;      /**< UI 권장 높이 */
  DeviceInfo deviceInfo; /**< 최신 센서 데이터 스냅샷 */
};

/**
 * @class CameraModel
 * @brief 그리드 시스템의 논리적 슬롯과 물리적 카메라를 중계하는
 * QAbstractListModel 서브클래스
 *
 * **Standard Usage Methodology:**
 * 1. 서버 전송 데이터 수신 시 onStoreUpdated()를 호출하여 전체 카메라의 온라인
 * 상태를 동기화합니다.
 * 2. 사용자가 UI에서 특정 카드를 '분할'하면 splitSlot()을 통해 하나의 소스를
 * 여러 타일로 쪼개어 노출합니다.
 * 3. `slotsUpdated` 시그널을 VideoManager가 감지하여 각 슬롯에 맞는 비디오
 * 디코더를 바인딩합니다.
 */
class CameraModel : public QAbstractListModel {
  Q_OBJECT
public:
  /** @brief QML 레이어에서 접근 가능한 데이터 역할(Role) 정의 */
  enum Roles {
    SlotIdRole = Qt::UserRole + 1, /**< 고유 슬롯 번호 */
    TitleRole,                     /**< 표시 이름 */
    IsOnlineRole,                  /**< 온라인 여부 */
    DescriptionRole,               /**< 상세 설명 */
    CardWidthRole,                 /**< 카드 너비 */
    CardHeightRole,                /**< 카드 높이 */
    CameraTypeRole,                /**< 기기 타입 */
    SplitCountRole,                /**< 분할 수 */
    CropRectRole,                  /**< 크롭 영역 (QRectF) */
    SplitDirectionRole,            /**< 분할 방향 */
    CameraIdRole                   /**< 원본 카메라 식별자 */
  };

  explicit CameraModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  QHash<int, QByteArray> roleNames() const override;

  void setCameraManager(CameraManager *mgr) { cameraManager_ = mgr; }

  /** @brief 두 슬롯의 위치(인덱스) 교체 */
  Q_INVOKABLE void swapSlots(int indexA, int indexB);

  /** @brief 특정 인덱스의 온라인 상태 강제 설정 */
  Q_INVOKABLE void setOnline(int index, bool online);

  /**
   * @brief 단일 슬롯을 여러 타일로 분할
   * @param rowIndex 분할 대상 행 번호
   * @param tileCount 분할할 개수
   * @param direction 분할 방향 (0:Col, 1:Row, 2:Grid)
   */
  Q_INVOKABLE void splitSlot(int rowIndex, int tileCount, int direction = 0);

  /** @brief 분할된 타일들을 하나의 슬롯으로 재통합 */
  Q_INVOKABLE void mergeSlots(int anyTileRowIndex, int tileCount);

  /** @return bool 해당 슬롯이 현재 분할된 상태인지 여부 */
  Q_INVOKABLE bool isSlotSplit(int slotId) const;

  /** @brief 설정 기반 자동 분할 규칙 적용 */
  Q_INVOKABLE bool applyAutoSplitForSlot(int slotId);

  /** @brief 전체 목록 초기화 */
  Q_INVOKABLE void clearAll();

  Q_INVOKABLE int rowForSlot(int slotId) const;
  Q_INVOKABLE bool hasSlot(int slotId) const;
  Q_INVOKABLE QString titleForSlot(int slotId) const;
  Q_INVOKABLE bool isOnlineForSlot(int slotId) const;
  Q_INVOKABLE QRectF cropRectForSlot(int slotId) const;
  Q_INVOKABLE QString cameraIdForSlot(int slotId) const;

  /**
   * @brief 특정 카메라의 최신 센서 정보 조회
   * @return QJsonObject {hum, light, tilt, tmp}
   */
  Q_INVOKABLE QJsonObject sensorInfoForCameraId(const QString &cameraId) const;

public slots:
  /** @brief 서버 데이터 스냅샷과 모델 동기화 */
  void refreshFromCameraManager();

  /** @brief 특정 카메라의 실시간 센서값 갱신 */
  void refreshSensorInfo();

signals:
  /** @brief 슬롯 구성이 변경되어 비디오 엔진의 재바인딩이 필요할 때 발생 */
  void slotsUpdated(QList<SlotInfo> slots);
  /** @brief 전체 고유 카메라 ID 목록이 변경됨을 알림 */
  void cameraIdsUpdated(QStringList cameraIds);
  /** @brief 특정 카메라가 오프라인에서 온라인으로 전환됨을 통지 */
  void cameraOnline(const QString &cameraId);
  /** @brief 센서 데이터가 업데이트되어 UI 갱신이 필요함을 알림 */
  void sensorDataUpdated(const QString &cameraId);

private:
  static QRectF computeCropRect(int tileIndex, int tileCount,
                                SplitDirection direction);
  void _emitSlotsUpdated();
  int _indexBySlotId(int slotId) const;
  bool _splitSlotAtIndex(int rowIndex, int tileCount, SplitDirection direction,
                         bool autoSplit);
  bool _mergeGroupByIndex(int anyTileRowIndex);
  bool _autoSplitForIndex(int rowIndex);

  int nextSlotId_ = 0;       /**< 신규 슬롯에 부여할 다음 시퀀스 */
  int nextSplitGroupId_ = 1; /**< 신규 분할 그룹 ID */

  CameraManager *cameraManager_ = nullptr;
  QList<CameraEntry> cameras_;        /**< 내부 데이터 저장소 */
  QSet<int> autoSplitAppliedSlotIds_; /**< 자동 분할이 이미 처리된 슬롯 목록 */
};

/**
 * @section Workflow Guide
 *
 * **[CameraModel 그리드 관리 워크플로우]**
 *
 * 1. 초기화: `onStoreUpdated`가 호출되면 서버에 존재하는 모든 카메라를 1:1
 * 슬롯으로 생성합니다.
 * 2. 분할 결정: 카메라의 종류가 어안(Fisheye)이거나 다중 채널인 경우 코어
 * 설정에 따라 `applyAutoSplitForSlot`이 실행되어 타일화됩니다.
 * 3. 렌더링 동기화: `slotsUpdated` 시그널이 발생하면 비디오
 * 엔진(VideoManager)은 각 슬롯 ID에 해당하는 `CropRect` 정보를 바탕으로 GPU
 * 셰이더 파라미터를 조정합니다.
 * 4. 텔레메트리 연동: 개별 카메라 카드(CameraCard.qml)는 `sensorDataUpdated`를
 * 구독하여 온도/기울기 등의 수치를 실시간으로 표시합니다.
 */
