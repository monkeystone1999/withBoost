#include "UserModel.hpp"

UserModel::UserModel(QObject *parent) : QAbstractListModel(parent) {}

int UserModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_users.size();
}

QVariant UserModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_users.size())
        return {};

    const UserEntry &user = m_users[index.row()];

    switch (role) {
    case UserIdRole:
        return user.userId;
    case UsernameRole:
        return user.username;
    case EmailRole:
        return user.email;
    case RoleRole:
        return user.role;
    case IsOnlineRole:
        return user.isOnline;
    case LastLoginRole:
        return user.lastLogin;
    case IpAddressRole:
        return user.ipAddress;
    case ActiveCamerasRole:
        return user.activeCameras;
    default:
        return {};
    }
}

QHash<int, QByteArray> UserModel::roleNames() const {
    return {
        {UserIdRole, "userId"},
        {UsernameRole, "username"},
        {EmailRole, "email"},
        {RoleRole, "role"},
        {IsOnlineRole, "isOnline"},
        {LastLoginRole, "lastLogin"},
        {IpAddressRole, "ipAddress"},
        {ActiveCamerasRole, "activeCameras"}
    };
}

void UserModel::addUser(const QString &userId, const QString &username,
                        const QString &email, const QString &role) {
    // Check if user already exists
    if (m_byId.contains(userId)) {
        int idx = m_byId[userId];
        m_users[idx].username = username;
        m_users[idx].email = email;
        m_users[idx].role = role;
        m_users[idx].isOnline = true;
        m_users[idx].lastLogin = QDateTime::currentDateTime();

        QModelIndex modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx);
        return;
    }

    int row = m_users.size();
    beginInsertRows(QModelIndex(), row, row);

    UserEntry entry;
    entry.userId = userId;
    entry.username = username;
    entry.email = email;
    entry.role = role;
    entry.isOnline = true;
    entry.lastLogin = QDateTime::currentDateTime();
    entry.activeCameras = 0;

    m_users.append(entry);
    m_byId[userId] = row;

    endInsertRows();
    emit countChanged();
    emit userAdded(userId);
}

void UserModel::removeUser(const QString &userId) {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return;

    beginRemoveRows(QModelIndex(), idx, idx);
    m_users.removeAt(idx);
    m_byId.remove(userId);

    // Rebuild index map
    m_byId.clear();
    for (int i = 0; i < m_users.size(); ++i) {
        m_byId[m_users[i].userId] = i;
    }

    endRemoveRows();
    emit countChanged();
    emit userRemoved(userId);
}

void UserModel::setUserOnline(const QString &userId, bool online) {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return;

    m_users[idx].isOnline = online;
    if (online) {
        m_users[idx].lastLogin = QDateTime::currentDateTime();
    }

    QModelIndex modelIdx = index(idx);
    emit dataChanged(modelIdx, modelIdx);
}

void UserModel::updateActiveCameras(const QString &userId, int count) {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return;

    m_users[idx].activeCameras = count;
    QModelIndex modelIdx = index(idx);
    emit dataChanged(modelIdx, modelIdx);
}

void UserModel::clearAll() {
    beginResetModel();
    m_users.clear();
    m_byId.clear();
    endResetModel();
    emit countChanged();
}

QString UserModel::getUsernameById(const QString &userId) const {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return QString();
    return m_users[idx].username;
}

bool UserModel::isUserAdmin(const QString &userId) const {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return false;
    return m_users[idx].role == "admin";
}

void UserModel::onStoreUpdated(std::vector<UserData> snapshot) {
    beginResetModel();
    m_users.clear();
    m_byId.clear();

    for (size_t i = 0; i < snapshot.size(); ++i) {
        const UserData &data = snapshot[i];

        UserEntry entry;
        entry.userId = QString::fromStdString(data.userId);
        entry.username = QString::fromStdString(data.username);
        entry.email = QString::fromStdString(data.email);
        entry.role = (data.role == UserRole::Admin) ? "admin" : "user";
        entry.isOnline = data.isOnline;
        entry.lastLogin = QDateTime::fromSecsSinceEpoch(
            std::chrono::system_clock::to_time_t(data.lastLogin));
        entry.ipAddress = QString::fromStdString(data.ipAddress);
        entry.activeCameras = data.activeCameras;

        m_users.append(entry);
        m_byId[entry.userId] = static_cast<int>(i);
    }

    endResetModel();
    emit countChanged();
}

int UserModel::findIndexByUserId(const QString &userId) const {
    auto it = m_byId.find(userId);
    if (it == m_byId.end())
        return -1;
    return it.value();
}
