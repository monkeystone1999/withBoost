#pragma once
#include <QQuickItem>
#include <QVariantList>

// ============================================================
//  DeviceGraphItem — C++ QQuickItem으로 구현한 라인 그래프
//
//  QML Canvas 방식 사용 (QSG 충돌 없음).
//  사용법:
//    DeviceGraph {
//        cpuHistory: deviceModel.getHistory(ip)
//        width: parent.width
//        height: 80
//    }
//
//  실제 렌더링은 QML Canvas delegate로 처리하되,
//  데이터 집계/정규화는 C++에서 수행.
// ============================================================
class DeviceGraphItem : public QQuickItem {
  Q_OBJECT
  Q_PROPERTY(
      QVariantList history READ history WRITE setHistory NOTIFY historyChanged)
  Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
  Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY
                 lineColorChanged)
  Q_PROPERTY(double minValue READ minValue NOTIFY historyChanged)
  Q_PROPERTY(double maxValue READ maxValue NOTIFY historyChanged)
  QML_ELEMENT

public:
  explicit DeviceGraphItem(QQuickItem *parent = nullptr);

  QVariantList history() const { return history_; }
  void setHistory(const QVariantList &h);

  QString label() const { return label_; }
  void setLabel(const QString &l);

  QColor lineColor() const { return lineColor_; }
  void setLineColor(const QColor &c);

  double minValue() const { return minValue_; }
  double maxValue() const { return maxValue_; }

  // 정규화된 값 배열 반환 [0.0 ~ 1.0] — Canvas에서 사용
  Q_INVOKABLE QVariantList normalizedValues() const;

signals:
  void historyChanged();
  void labelChanged();
  void lineColorChanged();

private:
  void recompute();

  QVariantList history_;
  QString label_;
  QColor lineColor_{0x44, 0xaa, 0xff};
  double minValue_{0.0};
  double maxValue_{100.0};
};
