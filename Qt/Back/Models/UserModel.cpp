#include "UserModel.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
  return {{UserIdRole, "userId"},       {UsernameRole, "username"},
          {EmailRole, "email"},         {RoleRole, "role"},
          {IsOnlineRole, "isOnline"},   {LastLoginRole, "lastLogin"},
          {IpAddressRole, "ipAddress"}, {ActiveCamerasRole, "activeCameras"}};
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

void UserModel::onPendingListReceived(const QString &json) {
  const auto doc = QJsonDocument::fromJson(json.toUtf8());
  if (!doc.isObject())
    return;

  const auto obj = doc.object();
  if (!obj.value("success").toBool(false))
    return;

  const auto usersArray = obj.value("users").toArray();

  beginResetModel();
  users_.clear();
  byId_.clear();

  for (int i = 0; i < usersArray.size(); ++i) {
    const auto u = usersArray[i].toObject();

    UserEntry entry;
    entry.userId =
        u.value("username").toString(); // Use username as ID for pending
    entry.username = u.value("username").toString();
    entry.email = u.value("email").toString();
    entry.role = "pending";
    entry.isOnline = false;
    entry.lastLogin =
        QDateTime::fromString(u.value("created").toString(), Qt::ISODateWithMs);
    if (!entry.lastLogin.isValid()) {
      entry.lastLogin = QDateTime::fromString(u.value("created").toString(),
                                              "yyyy-MM-dd HH:mm:ss");
    }
    entry.ipAddress = "";
    entry.activeCameras = 0;

    users_.append(entry);
    byId_[entry.userId] = i;
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
