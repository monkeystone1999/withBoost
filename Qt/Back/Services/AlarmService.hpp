#pragma once

class AlarmManager;
#include "../../Src/Domain/AlarmManager.hpp"
#include <QObject>
#include <QString>

/**
 * @file AlarmService.hpp
 * @brief 알람 이벤트 전파 및 Qt 시그널 변환 서비스 (Adapter)
 *
 * 이 파일은 코어 엔진(Domain::AlarmManager)에서 발생한 알람 이벤트를
 * Qt UI 레이어(AlarmController)로 안전하게 중계하는 서비스 클래스를 정의합니다.
 * 스레드 풀에서 가공된 원시 데이터를 GUI 스레드용 시그널로 변환합니다.
 */

class AlarmService : public QObject {
  Q_OBJECT
public:
  /**
   * @brief AlarmService 생성자
   * @param dispatcher 도메인 레이어의 알람 매니저 (non-owning)
   */
  explicit AlarmService(AlarmManager *dispatcher, QObject *parent = nullptr);
  ~AlarmService() override;

public slots:
  /**
   * @brief 코어 도메인 엔진으로부터 알람 이벤트 수신 (Thread-safe)
   * @param ev 파싱이 완료된 도메인 알람 이벤트 구조체
   * @note 이 슬롯은 QMetaObject::invokeMethod를 통해 GUI 스레드에서 안전하게
   * 실행됩니다.
   */
  void onAlarm(AlarmEvent ev);

signals:
  /**
   * @brief UI 컨트롤러로 전파되는 정형화된 알람 시그널
   * @param title 알람 명칭
   * @param detail 세부 발생 원인
   * @param severity 위험 레벨
   */
  void alarmTriggered(QString title, QString detail, int severity);

private:
  AlarmManager *dispatcher_; /**< 알람 소스 (Domain Layer) */
};

/**
 * @section Workflow Guide
 *
 * **[AlarmService 이벤트 중계 워크플로우]**
 *
 * 1. 데이터 생성: `NetworkManager`가 수신한 패킷을 `Domain::AlarmManager`가
 * 파싱하여 `AlarmEvent`를 생성합니다.
 * 2. 비동기 호출: 도메인 매니저는 `AlarmService::onAlarm`을
 * `Qt::QueuedConnection` 방식으로 예약 호출합니다.
 * 3. 시그널 발생: GUI 스레드에서 `onAlarm`이 실행될 때 `std::string` 데이터를
 * `QString`으로 변환하여 `alarmTriggered`를 발행합니다.
 */
