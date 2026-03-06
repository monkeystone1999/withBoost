#include "CameraModel.hpp"
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <utility>


CameraModel::CameraModel(QObject *parent) : QAbstractListModel(parent) {}

int CameraModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(m_cameras.size());
}

QVariant CameraModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_cameras.size())
    return {};

  const auto &cam = m_cameras[index.row()];
  switch (role) {
  case TitleRole:
    return cam.title;
  case RtspUrlRole:
    return cam.rtspUrl;
  case IsOnlineRole:
    return cam.isOnline;
  case DescriptionRole:
    return cam.description;
  case CardWidthRole:
    return cam.width;
  case CardHeightRole:
    return cam.height;
  default:
    return {};
  }
}

bool CameraModel::setData(const QModelIndex &index, const QVariant &value,
                          int role) {
  if (!index.isValid() || index.row() >= m_cameras.size())
    return false;

  auto &cam = m_cameras[index.row()];
  switch (role) {
  case TitleRole:
    cam.title = value.toString();
    break;
  case RtspUrlRole:
    cam.rtspUrl = value.toString();
    break;
  case IsOnlineRole:
    cam.isOnline = value.toBool();
    break;
  case DescriptionRole:
    cam.description = value.toString();
    break;
  case CardWidthRole:
    cam.width = value.toInt();
    break;
  case CardHeightRole:
    cam.height = value.toInt();
    break;
  default:
    return false;
  }
  emit dataChanged(index, index, {role});
  return true;
}

QHash<int, QByteArray> CameraModel::roleNames() const {
  return {{TitleRole, "title"},         {RtspUrlRole, "rtspUrl"},
          {IsOnlineRole, "isOnline"},   {DescriptionRole, "description"},
          {CardWidthRole, "cardWidth"}, {CardHeightRole, "cardHeight"}};
}

void CameraModel::addCamera(const QString &title, const QString &rtspUrl,
                            bool isOnline, const QString &description,
                            int width, int height) {
  beginInsertRows(QModelIndex(), m_cameras.size(), m_cameras.size());
  m_cameras.append({title, rtspUrl, isOnline, description, width, height});
  endInsertRows();
}

void CameraModel::swapCameraUrls(int indexA, int indexB) {
  if (indexA < 0 || indexB < 0 || indexA >= m_cameras.size() ||
      indexB >= m_cameras.size() || indexA == indexB)
    return;

  std::swap(m_cameras[indexA].rtspUrl, m_cameras[indexB].rtspUrl);
  std::swap(m_cameras[indexA].isOnline, m_cameras[indexB].isOnline);
  std::swap(m_cameras[indexA].title, m_cameras[indexB].title);

  const auto idxA = createIndex(indexA, 0);
  const auto idxB = createIndex(indexB, 0);
  emit dataChanged(idxA, idxA, {RtspUrlRole, IsOnlineRole, TitleRole});
  emit dataChanged(idxB, idxB, {RtspUrlRole, IsOnlineRole, TitleRole});
}

void CameraModel::setOnline(int index, bool online) {
  if (index < 0 || index >= m_cameras.size())
    return;
  m_cameras[index].isOnline = online;
  auto idx = createIndex(index, 0);
  emit dataChanged(idx, idx, {IsOnlineRole});
}

void CameraModel::refreshFromJson(const QString &jsonString) {
  QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
  if (!doc.isArray())
    return;

  const QJsonArray arr = doc.array();

  QHash<QString, int> localUrlCount;
  for (const auto &cam : m_cameras)
    localUrlCount[cam.rtspUrl] += 1;

  QHash<QString, int> remoteUrlCount;
  QHash<QString, bool> remoteOnlineByUrl;
  for (const auto &value : arr) {
    const QJsonObject obj = value.toObject();
    const QString url = obj["source_url"].toString();
    remoteUrlCount[url] += 1;
    remoteOnlineByUrl[url] = obj["is_online"].toBool();
  }

  const bool sameCameraSet =
      (arr.size() == m_cameras.size()) && (localUrlCount == remoteUrlCount);
  const bool needsFullReset = !sameCameraSet;

  if (needsFullReset) {
    beginResetModel();
    m_cameras.clear();
    for (int i = 0; i < arr.size(); ++i) {
      QJsonObject obj = arr[i].toObject();
      QString type = obj["type"].toString();
      QString ip = obj["ip"].toString();
      QString title = ip + " (" + type + ")";
      QString rtspUrl = obj["source_url"].toString();
      bool isOnline = obj["is_online"].toBool();
      m_cameras.append({title, rtspUrl, isOnline, "", 320, 240});
    }
    endResetModel();
  } else {
    for (int i = 0; i < m_cameras.size(); ++i) {
      const auto it = remoteOnlineByUrl.find(m_cameras[i].rtspUrl);
      const bool isOnline =
          (it != remoteOnlineByUrl.end()) ? it.value() : false;
      if (m_cameras[i].isOnline != isOnline) {
        m_cameras[i].isOnline = isOnline;
        auto idx = createIndex(i, 0);
        emit dataChanged(idx, idx, {IsOnlineRole});
      }
    }
  }

  // VideoManager 에 현재 URL 목록 알림
  QStringList urls;
  for (const auto &cam : m_cameras)
    urls << cam.rtspUrl;
  emit urlsUpdated(urls);
}
