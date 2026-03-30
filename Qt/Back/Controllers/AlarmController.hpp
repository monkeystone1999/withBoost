#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtTypes>
#include <string>

/**
 * @file AlarmController.hpp
 * @brief 알람 큐 관리 및 QML 연동 컨트롤러
 *
 * 이 파일은 백엔드에서 발생한 알람 이벤트를 QML UI 레이어(ListView 등)에 리스트
 * 형태로 노출하기 위한 컨트롤러 클래스를 정의합니다. 최대 20개의 최신 알람을
 * 유지하는 로직을 포함합니다.
 */

// struct AlarmEvent removed to avoid naming collision with Domain layer

/**
 * @class AlarmController
 * @brief 알람 이력 관리 및 QML 프로퍼티 바인딩 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 시스템 엔진으로부터 호출되는 onAlarm() 슬롯을 통해 새 이벤트를 수신합니다.
 * 2. 수신된 데이터는 내부 리스트(alarms_) 최상단(Prepend)에 추가되며, 최대
 * 개수를 초과하면 오래된 항목이 제거됩니다.
 * 3. QML은 `alarms` 프로필을 ListView의 모델로 바인딩하여 실시간으로 화면을
 * 갱신합니다.
 */
class AlarmController : public QObject {
  Q_OBJECT
  /** @brief QML ListView에 바인딩될 알람 리스트 프로퍼티 */
  Q_PROPERTY(QVariantList alarms READ alarms NOTIFY alarmsChanged)

public:
  static constexpr int kMaxAlarms = 20; /**< 화면에 유지할 최대 알람 개수 */

  explicit AlarmController(QObject *parent = nullptr);

  /** @return QVariantList QML에서 인식 가능한 맵 리스트 형태로 반환 */
  QVariantList alarms() const;

public slots:
  /**
   * @brief 특정 알람 항목 삭제 (사용자 Dismiss)
   * @param id 삭제할 알람의 고유 ID
   */
  void dismiss(qint64 id);

  /** @brief 리스트 내의 모든 알람 삭제 */
  void clearAll();

  /**
   * @brief 신규 알람 수신 핸들러 (AlarmService 시그널 수신)
   * @param title 알람 제목
   * @param detail 세부 내용
   * @param severity 위험도 레벨
   */
  void onAlarm(QString title, QString detail, int severity);

signals:
  /** @brief QML 상태 갱신 통지 시그널 */
  void alarmsChanged();

private:
  /** @brief 내부 관리용 알람 엔트리 구조체 */
  struct AlarmEntry {
    qint64 id;
    QString title;
    QString detail;
    int severity;
  };

  QList<AlarmEntry> alarms_; /**< 메모리 내 상주 알람 리스트 */
};

/**
 * @section Workflow Guide
 *
 * **[AlarmController 통합 워크플로우]**
 *
 * 1. 수신: `AlarmManager`가 분석한 데이터가 `onAlarm` 슬롯을 통해 전달됩니다.
 * 2. 가공: 타임스탬프 기반의 고유 ID를 부여하고 `QVariantList`로 변환할 준비를
 * 합니다.
 * 3. 출력: `alarmsChanged()` 시그널에 의해 QML ListView가 재렌더링되며
 * 사용자에게 노출됩니다.
 */
