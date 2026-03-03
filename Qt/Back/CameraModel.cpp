#include "CameraModel.hpp"

CameraModel::CameraModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int CameraModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_cameras.size());
}

QVariant CameraModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_cameras.size())
        return {};

    const auto& cam = m_cameras[index.row()];
    switch (role) {
    case TitleRole:    return cam.title;
    case RtspUrlRole:  return cam.rtspUrl;
    case IsOnlineRole: return cam.isOnline;
    default:           return {};
    }
}

bool CameraModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() >= m_cameras.size())
        return false;

    auto& cam = m_cameras[index.row()];
    switch (role) {
    case TitleRole:    cam.title    = value.toString(); break;
    case RtspUrlRole:  cam.rtspUrl  = value.toString(); break;
    case IsOnlineRole: cam.isOnline = value.toBool();   break;
    default: return false;
    }
    emit dataChanged(index, index, {role});
    return true;
}

QHash<int, QByteArray> CameraModel::roleNames() const {
    return {
        { TitleRole,    "title"    },
        { RtspUrlRole,  "rtspUrl"  },
        { IsOnlineRole, "isOnline" }
    };
}

void CameraModel::addCamera(const QString& title, const QString& rtspUrl, bool isOnline) {
    beginInsertRows(QModelIndex(), m_cameras.size(), m_cameras.size());
    m_cameras.append({ title, rtspUrl, isOnline });
    endInsertRows();
}

void CameraModel::swapCameraUrls(int indexA, int indexB) {
    if (indexA < 0 || indexB < 0
        || indexA >= m_cameras.size()
        || indexB >= m_cameras.size()
        || indexA == indexB)
        return;

    std::swap(m_cameras[indexA].rtspUrl, m_cameras[indexB].rtspUrl);

    auto idxA = createIndex(indexA, 0);
    auto idxB = createIndex(indexB, 0);
    emit dataChanged(idxA, idxA, { RtspUrlRole });
    emit dataChanged(idxB, idxB, { RtspUrlRole });
}

void CameraModel::setOnline(int index, bool online) {
    if (index < 0 || index >= m_cameras.size()) return;
    m_cameras[index].isOnline = online;
    auto idx = createIndex(index, 0);
    emit dataChanged(idx, idx, { IsOnlineRole });
}
