#pragma once
#include <QAbstractListModel>
#include <QList>
#include <QObject>
#include <QString>

struct CameraEntry {
  QString title;
  QString rtspUrl;
  bool isOnline;
  QString description;
  int width;
  int height;
  QString cameraType;
};

class CameraModel : public QAbstractListModel {
  Q_OBJECT
public:
  enum Roles {
    TitleRole = Qt::UserRole + 1,
    RtspUrlRole,
    IsOnlineRole,
    DescriptionRole,
    CardWidthRole,
    CardHeightRole,
    CameraTypeRole
  };

  explicit CameraModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void addCamera(const QString &title, const QString &rtspUrl,
                             bool isOnline = false,
                             const QString &description = "", int width = 320,
                             int height = 240);
  Q_INVOKABLE void swapCameraUrls(int indexA, int indexB);
  Q_INVOKABLE void setOnline(int index, bool online);

public slots:
  void refreshFromJson(const QString &jsonString);

signals:
  void urlsUpdated(QStringList urls);

private:
  QList<CameraEntry> m_cameras;
};
