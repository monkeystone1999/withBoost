#pragma once
#include "../../../Src/Domain/StatusManager.hpp"
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <string>
#include <vector>

/**
 * @file ServerStatus.hpp
 * @brief 서버 시스템 자원 모니터링 모델
 */

/**
 * @class ServerStatusModel
 * @brief 서버 자원 상태 및 히스토리를 QML 레이어에 노출하는 서비스 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 코어 엔진의 데이터 수집기(`ServerStatusStore`)로부터 주기적으로
 * `ServerStatusData`를 수신합니다.
 * 2. `onStoreUpdated()` 슬롯에서 데이터를 가공하여 `serverHistory_`에 최근
 * 60개의 샘플을 보관합니다.
 * 3. QML의 가로형 차트 등은 `getServerHistory()`를 호출하여 서버의 부하 변화
 * 추이를 실시간으로 렌더링합니다.
 */
class ServerStatusModel : public QObject {
  Q_OBJECT
  /** @brief 서버 CPU 사용율 프로퍼티 */
  Q_PROPERTY(double serverCpu READ serverCpu NOTIFY statusUpdated)
  /** @brief 서버 메모리 사용율 프로퍼티 */
  Q_PROPERTY(double serverMemory READ serverMemory NOTIFY statusUpdated)
  /** @brief 서버 온도 프로퍼티 */
  Q_PROPERTY(double serverTemp READ serverTemp NOTIFY statusUpdated)
  /** @brief 서버 상태 유효 여부 (권한 및 연결 상태 포함) */
  Q_PROPERTY(bool hasServer READ hasServer NOTIFY statusUpdated)
public:
  explicit ServerStatusModel(QObject *parent = nullptr);

  double serverCpu() const {
    return statusManager_ && !statusManager_->getStatus().cpu.empty()
               ? statusManager_->getStatus().cpu.back()
               : 0.0;
  }
  double serverMemory() const {
    return statusManager_ && !statusManager_->getStatus().memory.empty()
               ? statusManager_->getStatus().memory.back()
               : 0.0;
  }
  double serverTemp() const {
    return statusManager_ && !statusManager_->getStatus().temp.empty()
               ? statusManager_->getStatus().temp.back()
               : 0.0;
  }
  bool hasServer() const { return statusManager_ != nullptr; }

  void setStatusManager(StatusManager *mgr) { statusManager_ = mgr; }

  /** @brief 특정 장치의 실시간 부하 정보 조회 */
  Q_INVOKABLE double deviceCpu(const QString &ip) const;
  Q_INVOKABLE double deviceMemory(const QString &ip) const;
  Q_INVOKABLE double deviceTemp(const QString &ip) const;
  Q_INVOKABLE int deviceUptime(const QString &ip) const;

  /** @brief 서버 자원 변화 이력 반환 (차트 렌더링용) */
  Q_INVOKABLE QVariantList getServerHistory() const;

public slots:
  /** @brief 코어 엔진으로부터 최신 상태 스냅샷 업데이트 */
  void refreshFromStatusManager();

signals:
  /** @brief 모든 상태 데이터가 갱신되었음을 알림 */
  void statusUpdated();

private:
  StatusManager *statusManager_ = nullptr;
  QVariantList serverHistory_; /**< 시계열 히스토리 저장소 (최대 60개) */
};

/**
 * @section Workflow Guide
 *
 * **[ServerStatus 모니터링 워크플로우]**
 *
 * 1. 데이터 수령: 네트워크 스택이 서버의 시스템 자원 텔레메트리 패킷을 파싱하면
 * 코어가 GUI 스레드로 `onStoreUpdated`를 호출합니다.
 * 2. 모델 갱신: 서버 상태 정보를 필드별로 업데이트하고, 신규 데이터를
 * `serverHistory_` 큐에 밀어 넣습니다.
 * 3. 대시보드 반영: `statusUpdated` 시그널에 의해 메인
 * 대시보드(Dashboard.qml)의 원형 게이지와 꺾은선 그래프가 즉시 다시 그려집니다.
 * 4. 장치 상세 조회: 개별 카메라 설정 페이지 등에서 기기 IP를 키로 하여 해당
 * 장치의 부하 정보를 실시간으로 조회합니다.
 */
