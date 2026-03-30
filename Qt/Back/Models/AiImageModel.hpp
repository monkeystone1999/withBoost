#pragma once

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

/**
 * @file AiImageModel.hpp
 * @brief AI 분석 이미지 데이터 모델
 *
 * 이 파일은 서버에서 전송된 객체 검출 및 트래킹 결과 이미지(Base64 형식)를
 * 각 카메라 ID별로 분류하여 저장하고, QML UI에서 접근할 수 있는 인터페이스를
 * 제공합니다.
 */

/**
 * @struct AiImageEvent
 * @brief AI 서버로부터 수신된 단일 분석 프레임 이벤트 정보
 */
struct AiImageEvent {
  QString cameraId;        /**< 카메라 식별자 ("IP/StreamIndex") */
  QString deviceName;      /**< 소속 장치 명칭 */
  int trackId = 0;         /**< 객체 트래킹 고유 ID */
  int frameIndex = 0;      /**< 전체 분석 시퀀스 내 프레임 번호 */
  int totalFrames = 0;     /**< 시퀀스의 총 프레임 수 */
  long long timestamp = 0; /**< 발생 시각 (Unix Timestamp) */
  QString base64Image;     /**< JPEG 이미지를 Base64로 인코딩한 문자열 */
};

/**
 * @class AiImageModel
 * @brief 카메라별 AI 이벤트 이력 관리 및 조회 클래스
 *
 * **Standard Usage Methodology:**
 * 1. AI 분석 서버로부터 데이터 수신 시 onImageReceivedBase64()가 호출되어 내부
 * 저장소에 누적됩니다.
 * 2. QML UI는 `aiEventReceived` 시그널을 수신하면 getLatestEvent()를 통해 최신
 * 분석 이미지를 가져와 화면에 표시합니다.
 * 3. 분석 이력 화면에서는 getEventsForCamera()를 사용하여 특정 카메라의 과거
 * 이력을 리스트로 표시합니다.
 */
class AiImageModel : public QObject {
  Q_OBJECT
public:
  explicit AiImageModel(QObject *parent = nullptr);
  ~AiImageModel();

  /**
   * @brief 특정 카메라의 모든 저장된 이벤트 목록 조회
   * @param cameraId 조회할 카메라 ID
   * @return QVariantList AiImageEvent 맵들의 리스트
   */
  Q_INVOKABLE QVariantList getEventsForCamera(const QString &cameraId) const;

  /**
   * @brief 특정 카메라의 가장 최근 이벤트 조회
   * @param cameraId 조회할 카메라 ID
   * @return QVariantMap 최신 이벤트 데이터를 담은 맵
   */
  Q_INVOKABLE QVariantMap getLatestEvent(const QString &cameraId) const;

  /**
   * @brief 특정 IP 주소와 관련된 최근 이벤트 검색 (레거시 지원)
   * @param ip 검색 대상 IP 주소
   */
  Q_INVOKABLE QVariantList getRecentEvents(const QString &ip) const;

  static constexpr int MAX_EVENTS_PER_CAMERA =
      20; /**< 카메라당 메모리에 유지할 최대 이벤트 수 */

public slots:
  /**
   * @brief 신규 AI 분석 결과 수신 등록 (Base64 형식)
   * @param cameraId 카메라 식별자
   * @param deviceName 장치명
   * @param trackId 트래킹 ID
   * @param frameIndex 현재 프레임 번호
   * @param totalFrames 총 프레임
   * @param timestamp 시간
   * @param base64Image 인코딩된 이미지 데이터
   */
  void onImageReceivedBase64(const QString &cameraId, const QString &deviceName,
                             int trackId, int frameIndex, int totalFrames,
                             long long timestamp, const QString &base64Image);

  /** @brief 모든 카메라의 이벤트 이력 초기화 */
  void clearAll();

signals:
  /** @brief 특정 카메라의 새로운 분석 결과가 모델에 추가되었음을 알림 */
  void aiEventReceived(const QString &cameraId);

private:
  QMap<QString, QList<AiImageEvent>>
      eventsByCameraId_; /**< ID 기반 이벤트 큐 저장소 */
};

/**
 * @section Workflow Guide
 *
 * **[AiImageModel 분석 데이터 워크플로우]**
 *
 * 1. 데이터 수집: 네트워크 엔진이 수신한 `MessageType::AI_IMAGE` 패킷이
 * `onImageReceivedBase64`로 전달됩니다.
 * 2. 큐 관리: 카메라별로 최대 20개까지만 유지하며, 초과 시 가장 오래된 이벤트를
 * 자동 삭제하여 메모리를 관리합니다.
 * 3. UI 바인딩: `aiEventReceived` 시그널을 받은 QML 소품(Item)들이
 * `base64Image`를 `Image` 요소의 소스로 즉시 반영합니다.
 */
