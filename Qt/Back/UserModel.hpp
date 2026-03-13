#pragma once
#include "../../Src/Domain/User.hpp"
#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <vector>

struct UserEntry {
    QString userId;
    QString username;
    QString email;
    QString role;  // "user" or "admin"
    bool isOnline;
    QDateTime lastLogin;
    QString ipAddress;
    int activeCameras;

    UserEntry()
        : isOnline(false)
        , activeCameras(0) {}
};

class UserModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        UserIdRole = Qt::UserRole + 1,
        UsernameRole,
        EmailRole,
        RoleRole,
        IsOnlineRole,
        LastLoginRole,
        IpAddressRole,
        ActiveCamerasRole
    };

    explicit UserModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return users_.count(); }

    Q_INVOKABLE void addUser(const QString &userId, const QString &username,
                             const QString &email, const QString &role);
    Q_INVOKABLE void removeUser(const QString &userId);
    Q_INVOKABLE void setUserOnline(const QString &userId, bool online);
    Q_INVOKABLE void updateActiveCameras(const QString &userId, int count);
    Q_INVOKABLE void clearAll();

    Q_INVOKABLE QString getUsernameById(const QString &userId) const;
    Q_INVOKABLE bool isUserAdmin(const QString &userId) const;

public slots:
    void onStoreUpdated(std::vector<UserData> snapshot);

signals:
    void countChanged();
    void userAdded(const QString &userId);
    void userRemoved(const QString &userId);

private:
    int findIndexByUserId(const QString &userId) const;

    QList<UserEntry> users_;
    QHash<QString, int> byId_;  // userId -> row index
};
