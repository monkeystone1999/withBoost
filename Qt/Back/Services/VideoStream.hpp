#pragma once

class VideoEngine;
class CameraManager;

#include "Models/CameraModel.hpp"
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <atomic>
#include <functional>
#include <mutex>

/**
 * @file VideoStream.hpp
 * @brief 실시간 비디오 스트리밍 제어 및 QML 렌더링 인터페이스
 *
 * 이 파일은 코어 엔진의 비디오 데이터(NV12)를 Qt 하드웨어 가속 렌더링
 * 시스템(QVideoSink)으로 전달하기 위한 워커, 매니저 및 브리지 클래스들을
 * 정의합니다.
 */

/**
 * @class VideoWorker
 * @brief 개별 카메라 채널의 비디오 스트림 수신 및 프레임 버퍼링 워커
 *
 * 코어의 VideoEngine과 1:1로 대응하며, 수신된 원시 데이터를 QVideoFrame으로
 * 변환하여 스레드 안전하게 보관합니다.
 */
class VideoWorker : public QObject {
  Q_OBJECT
public:
  explicit VideoWorker(const QString &cameraId, QObject *parent = nullptr);
  ~VideoWorker() override;

  /** @brief 스트리밍 시작 (VideoEngine 기동) */
  void startStream();
  /** @brief 스트리밍 중지 및 리소스 해제 */
  void stopStream();

  /** @brief 비디오 소스 연결 URL 설정 */
  void setConnectionString(const QString &url) { connectionString_ = url; }
  /** @brief 하위 엔진 인스턴스 주입 */
  void setVideoEngine(VideoEngine *engine) { videoEngine_ = engine; }
  /** @brief 출력 프레임 레이트 제한 설정 */
  void setFpsLimit(int fps);

  /** @brief 최근 수신 프레임의 시퀀스 번호 조회 */
  uint64_t frameSeq() const {
    return frameSeq_.load(std::memory_order_acquire);
  }

  /**
   * @brief 최신 비디오 프레임 추출
   * @return 복사된 QVideoFrame 객체
   * @note 스핀락을 사용하여 렌더링 스레드와의 데이터 경합을 방지합니다.
   */
  QVideoFrame getLatestFrame() {
    while (frameSpinLock_.test_and_set(std::memory_order_acquire)) {
    }
    QVideoFrame f = latestFrame_;
    frameSpinLock_.clear(std::memory_order_release);
    return f;
  }

signals:
  /** @brief 신규 프레임 준비 알림 (Event-driven 방식용) */
  void frameReady(const QVideoFrame &frame);

private:
  QString cameraId_;
  QString connectionString_;
  VideoEngine *videoEngine_ = nullptr;
  std::atomic<uint64_t> frameSeq_{0};
  std::atomic<bool> loggedFrameInfo_{false};
  int fpsLimit_ = 30;

  std::atomic_flag frameSpinLock_ = ATOMIC_FLAG_INIT;
  QVideoFrame latestFrame_;
};

/**
 * @class VideoManager
 * @brief 시스템 내 비디오 워커 인스턴스들을 총괄 관리하는 싱글톤급 관리자
 *
 * 카메라 ID 또는 UI 슬롯 ID를 기반으로 워커를 할당하고 스트림의 생명주기를
 * 관리합니다.
 */
class VideoManager : public QObject {
  Q_OBJECT
public:
  explicit VideoManager(QObject *parent = nullptr);
  ~VideoManager() override;

  /** @brief 카메라 ID를 통한 워커 조회 */
  Q_INVOKABLE VideoWorker *getWorker(const QString &cameraId);
  /** @brief UI 슬롯 번호를 통한 워커 조회 */
  Q_INVOKABLE VideoWorker *getWorkerBySlot(int slotId);
  /** @brief 특정 슬롯에 할당된 카메라 유무 확인 */
  Q_INVOKABLE bool hasSlot(int slotId) const;
  /** @brief 슬롯 번호에 대응하는 카메라 ID 반환 */
  Q_INVOKABLE QString slotCameraId(int slotId) const;

  /** @brief 카메라별 스트리밍 URL 제공 콜백 등록 */
  void setUrlProvider(std::function<QString(const QString &)> provider) {
    urlProvider_ = std::move(provider);
  }
  /** @brief 기본 FPS 설정 제공 콜백 등록 */
  void setFpsProvider(std::function<int()> provider) {
    fpsProvider_ = std::move(provider);
  }
  /** @brief 코어 카메라 매니저 참조 주입 */
  void setCameraManager(CameraManager *mgr) { cameraManager_ = mgr; }

public slots:
  /** @brief UI 그리드 설계에 따른 카메라-슬롯 매핑 등록 및 스트림 시작 */
  void registerSlots(const QList<SlotInfo> &slots);
  /** @brief 카메라 리스트 일괄 등록 및 워커 생성 */
  void registerCameraIds(const QStringList &cameraIds);
  /** @brief 스트림 재연결 처리 */
  void restartWorker(const QString &cameraId);
  /** @brief 시스템 전체 FPS 제한 변경 */
  void setFpsLimit(int fps);
  /** @brief 모든 워커 정지 및 메모리 해제 */
  void clearAll();

signals:
  /** @brief 신규 워커 등록 완료 알림 */
  void workerRegistered(const QString &cameraId);

private:
  CameraManager *cameraManager_ = nullptr;
  QMap<QString, VideoWorker *> workers_;
  QMap<int, QString> slotToCameraId_;
  std::function<QString(const QString &)> urlProvider_;
  std::function<int()> fpsProvider_;
};

/**
 * @class VideoStream
 * @brief QML VideoOutput과 VideoWorker 사이를 연결하는 데이터 전송 브리지
 *
 * **Standard Usage Methodology:**
 * 1. QML에서 VideoOutput 인스턴스에 VideoStream을 프로퍼티로 할당합니다.
 * 2. slotId 또는 cameraId를 설정하여 특정 카메라 스트림과 바인딩합니다.
 * 3. 내부 타이머(16ms)가 VideoWorker의 시퀀스 변화를 체크하여 최신 프레임을
 * VideoSink로 푸시합니다.
 */
class VideoStream : public QObject {
  Q_OBJECT
  /** @brief Qt 비디오 렌더링 목적지 (QML VideoOutput 내부 객체) */
  Q_PROPERTY(QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY
                 videoSinkChanged)
  /** @brief 바인딩할 UI 슬롯 번호 */
  Q_PROPERTY(int slotId READ slotId WRITE setSlotId NOTIFY slotIdChanged)
  /** @brief 바인딩할 카메라 고유 아이디 */
  Q_PROPERTY(
      QString cameraId READ cameraId WRITE setCameraId NOTIFY cameraIdChanged)

public:
  explicit VideoStream(QObject *parent = nullptr);
  ~VideoStream() override;

  QVideoSink *videoSink() const { return m_videoSink.data(); }
  void setVideoSink(QVideoSink *sink);

  int slotId() const { return m_slotId; }
  void setSlotId(int id);

  QString cameraId() const { return m_cameraId; }
  void setCameraId(const QString &id);

signals:
  void videoSinkChanged();
  void slotIdChanged();
  void cameraIdChanged();

private slots:
  /** @brief 현재 설정된 ID/슬롯 정보를 바탕으로 워커 연결 시도 */
  void tryAttach();
  /** @brief 워커가 신규 등록되었을 때 연결 재시도 */
  void onWorkerRegistered(const QString &id);

protected:
  /** @brief 주기적인 시퀀스 정합성 체크 및 프레임 푸시 루프 */
  void timerEvent(QTimerEvent *event) override;

private:
  QPointer<QVideoSink> m_videoSink;
  int m_slotId = -1;
  QString m_cameraId;
  QPointer<VideoWorker> m_worker; /**< 현재 바인딩된 비디오 워커 */

  int m_timerId = 0;
  uint64_t m_lastSeq = 0; /**< 마지막으로 렌더링된 프레임 번호 */
};

/**
 * @section Workflow Guide
 *
 * **[비디오 스트리밍 파이프라인 워크플로우]**
 *
 * 1. 시스템 부팅: `VideoManager`가 생성되고 앱 컨트롤러로부터 카메라-슬롯 매핑
 * 정보를 수신하여 `VideoWorker`를 한꺼번에 기동합니다.
 * 2. 원시 데이터 처리: `VideoWorker`의 콜백 함수가 코어 엔진의 NV12 바이트
 * 스트림을 수신하여 `QVideoFrame`의 플레인(Y, UV)으로 복사합니다.
 * 3. 렌더링 바인딩: 사용자가 QML 화면을 전환하면 `VideoStream::setSlotId`가
 * 호출되고, 해당 슬롯의 `VideoWorker`를 찾아 내부 포인터로 연결합니다.
 * 4. 프레임 푸시: `VideoStream`의 타이머가 `m_worker->frameSeq()` 변화를
 * 감지하면, `getLatestFrame()`을 통해 복사본을 가져와 `QVideoSink`에 전달하여
 * 매끄러운 영상을 출력합니다.
 */
