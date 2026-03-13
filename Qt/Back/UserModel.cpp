#include "UserModel.hpp"

UserModel::UserModel(QObject *parent) : QAbstractListModel(parent) {}

int UserModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return users_.size();
}

QVariant UserModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= users_.size())
        return {};

    const UserEntry &user = users_[index.row()];

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
    if (byId_.contains(userId)) {
        int idx = byId_[userId];
        users_[idx].username = username;
        users_[idx].email = email;
        users_[idx].role = role;
        users_[idx].isOnline = true;
        users_[idx].lastLogin = QDateTime::currentDateTime();

        QModelIndex modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx);
        return;
    }

    int row = users_.size();
    beginInsertRows(QModelIndex(), row, row);

    UserEntry entry;
    entry.userId = userId;
    entry.username = username;
    entry.email = email;
    entry.role = role;
    entry.isOnline = true;
    entry.lastLogin = QDateTime::currentDateTime();
    entry.activeCameras = 0;

    users_.append(entry);
    byId_[userId] = row;

    endInsertRows();
    emit countChanged();
    emit userAdded(userId);
}

void UserModel::removeUser(const QString &userId) {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return;

    beginRemoveRows(QModelIndex(), idx, idx);
    users_.removeAt(idx);
    byId_.remove(userId);

    // Rebuild index map
    byId_.clear();
    for (int i = 0; i < users_.size(); ++i) {
        byId_[users_[i].userId] = i;
    }

    endRemoveRows();
    emit countChanged();
    emit userRemoved(userId);
}

void UserModel::setUserOnline(const QString &userId, bool online) {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return;

    users_[idx].isOnline = online;
    if (online) {
        users_[idx].lastLogin = QDateTime::currentDateTime();
    }

    QModelIndex modelIdx = index(idx);
    emit dataChanged(modelIdx, modelIdx);
}

void UserModel::updateActiveCameras(const QString &userId, int count) {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return;

    users_[idx].activeCameras = count;
    QModelIndex modelIdx = index(idx);
    emit dataChanged(modelIdx, modelIdx);
}

void UserModel::clearAll() {
    beginResetModel();
    users_.clear();
    byId_.clear();
    endResetModel();
    emit countChanged();
}

QString UserModel::getUsernameById(const QString &userId) const {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return QString();
    return users_[idx].username;
}

bool UserModel::isUserAdmin(const QString &userId) const {
    int idx = findIndexByUserId(userId);
    if (idx < 0)
        return false;
    return users_[idx].role == "admin";
}

void UserModel::onStoreUpdated(std::vector<UserData> snapshot) {
    beginResetModel();
    users_.clear();
    byId_.clear();

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

        users_.append(entry);
        byId_[entry.userId] = static_cast<int>(i);
    }

    endResetModel();
    emit countChanged();
}

int UserModel::findIndexByUserId(const QString &userId) const {
    auto it = byId_.find(userId);
    if (it == byId_.end())
        return -1;
    return it.value();
}
