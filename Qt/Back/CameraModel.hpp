#pragma once
#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QObject>

struct CameraEntry {
    QString title;
    QString rtspUrl;
    bool    isOnline;
};

class CameraModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        TitleRole   = Qt::UserRole + 1,
        RtspUrlRole,
        IsOnlineRole
    };

    explicit CameraModel(QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool     setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addCamera(const QString& title, const QString& rtspUrl, bool isOnline = false);
    Q_INVOKABLE void swapCameraUrls(int indexA, int indexB);
    Q_INVOKABLE void setOnline(int index, bool online);

signals:
    void alarmTriggered(const QString& title, const QString& detail, int severity);

private:
    QList<CameraEntry> m_cameras;
};
