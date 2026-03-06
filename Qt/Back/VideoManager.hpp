#pragma once

#include <QMap>
#include <QObject>
#include <QStringList>

#include "VideoWorker.hpp"

// VideoWorker 들의 영속 소유자.
// QML 에서 context property "videoManager" 로 접근한다.
// URL 기반으로 VideoWorker 를 관리하며 한 번 시작된 스트림은 프로그램 종료까지
// 유지된다.
class VideoManager : public QObject {
  Q_OBJECT

public:
  explicit VideoManager(QObject *parent = nullptr);
  ~VideoManager() override;

  // QML 에서 URL 로 VideoWorker 조회
  Q_INVOKABLE VideoWorker *getWorker(const QString &rtspUrl);

  // 모든 VideoWorker 종료 및 삭제 (로그아웃 시 호출)
  Q_INVOKABLE void clearAll();

signals:
  void workerRegistered(const QString &rtspUrl);

public slots:
  // CameraModel::urlsUpdated 와 연결한다.
  // 새로운 URL 에 대해서만 VideoWorker 를 생성하고 startStream() 호출.
  void registerUrls(const QStringList &urls);

private:
  QMap<QString, VideoWorker *> m_workers;
};
