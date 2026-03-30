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

  const UserMeta &user = users_[index.row()];

  switch (role) {
  case UserIdRole:
    return QString::fromStdString(user.name_);
  case UsernameRole:
    return QString::fromStdString(user.name_);
  case RoleRole:
    return user.state_ == UserState::Admin ? "admin" : "user";
  case IsOnlineRole:
    return true; // If in list, assume online for now as per StatusManager logic
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

void UserModel::refreshFromStatusManager() {
  if (!statusManager_)
    return;

  auto &srcUsers = statusManager_->getUsers();

  // Create a fast-lookup map for the new snapshot
  QHash<QString, UserMeta> newUsers;
  for (const auto &[id, meta] : srcUsers) {
    newUsers.insert(QString::fromStdString(meta.name_), meta);
  }

  bool changedCount = false;
  QSet<QString> existingNames;

  for (int i = 0; i < users_.size(); ++i) {
    auto &entry = users_[i];
    QString entryName = QString::fromStdString(entry.name_);
    existingNames.insert(entryName);
    auto it = newUsers.find(entryName);

    bool userChanged = false;
    QList<int> changedRoles;

    if (it == newUsers.end()) {
      // User no longer in Src manager
    } else {
      const auto &meta = it.value();

      if (entry.state_ != meta.state_) {
        entry.state_ = meta.state_;
        userChanged = true;
        changedRoles << RoleRole;
      }
    }

    if (userChanged) {
      emit dataChanged(createIndex(i, 0), createIndex(i, 0), changedRoles);
    }
  }

  QList<UserMeta> toAdd;
  for (auto it = newUsers.constBegin(); it != newUsers.constEnd(); ++it) {
    if (!existingNames.contains(it.key())) {
      toAdd.append(it.value());
    }
  }

  if (!toAdd.isEmpty()) {
    beginInsertRows(QModelIndex(), users_.size(),
                    users_.size() + toAdd.size() - 1);
    for (const auto &m : toAdd) {
      users_.append(m);
      byId_.insert(QString::fromStdString(m.name_), users_.size() - 1);
    }
    endInsertRows();
    changedCount = true;
  }

  if (changedCount) {
    emit countChanged();
  }
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
  return QString::fromStdString(users_[idx].name_);
}

bool UserModel::isUserAdmin(const QString &userId) const {
  int idx = findIndexByUserId(userId);
  if (idx < 0)
    return false;
  return users_[idx].state_ == UserState::Admin;
}

int UserModel::findIndexByUserId(const QString &userId) const {
  auto it = byId_.find(userId);
  if (it == byId_.end())
    return -1;
  return it.value();
}

/**
 * @section Workflow Guide
 *
 * **[UserModel 핵심 운영 가이드]**
 *
 * 1. 실시간 동기화 전략 (Full-Reset Pattern):
 *    - `onStoreUpdated` 호출 시 `beginResetModel()`과 `endResetModel()`을
 * 사용하여 전체 사용자 목록을 원자적으로 교체합니다.
 *    - 이는 사용자 목록이 빈번하게 바뀌지 않는 관리 시스템의 특성을 고려하여
 * 구현의 단순성과 데이터 일관성을 확보하기 위함입니다.
 *
 * 2. 권한 등급 처리:
 *    - `role` 필드는 단순 문자열("admin", "user")로 처리되나, 앱 전체의 보안
 * 로직에서 핵심적인 역할을 수행하므로 권한 검증 시 `isUserAdmin()` 도우미 함수
 * 사용을 권장합니다.
 *
 * 3. 인덱싱 최적화:
 *    - `byId_` 해시 맵은 UID를 키로 하여 모델 인덱스를 즉시 조회할 수 있게
 * 하여, 특정 사용자의 행위(로그인/로그아웃 등)를 시각화할 때 성능 병목을
 * 제거합니다.
 */
