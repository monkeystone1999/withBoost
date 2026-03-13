#include "DeviceGraphItem.hpp"
#include <QVariantMap>
#include <algorithm>
#include <limits>

DeviceGraphItem::DeviceGraphItem(QQuickItem *parent) : QQuickItem(parent) {}

void DeviceGraphItem::setHistory(const QVariantList &h) {
  if (history_ == h)
    return;
  history_ = h;
  recompute();
  emit historyChanged();
}

void DeviceGraphItem::setLabel(const QString &l) {
  if (label_ == l)
    return;
  label_ = l;
  emit labelChanged();
}

void DeviceGraphItem::setLineColor(const QColor &c) {
  if (lineColor_ == c)
    return;
  lineColor_ = c;
  emit lineColorChanged();
}

void DeviceGraphItem::recompute() {
  if (history_.isEmpty()) {
    minValue_ = 0.0;
    maxValue_ = 100.0;
    return;
  }

  double mn = std::numeric_limits<double>::max();
  double mx = std::numeric_limits<double>::lowest();

  for (const auto &v : qAsConst(history_)) {
    double val = 0.0;
    if (v.canConvert<QVariantMap>()) {
      // DeviceIntegrated history object — label별 필드 추출
      auto map = v.toMap();
      if (map.contains("cpu"))
        val = map["cpu"].toDouble();
      else if (map.contains("memory"))
        val = map["memory"].toDouble();
      else if (map.contains("temp"))
        val = map["temp"].toDouble();
      else
        val = map.first().toDouble();
    } else {
      val = v.toDouble();
    }
    mn = std::min(mn, val);
    mx = std::max(mx, val);
  }

  // 최소 범위 보장 (같은 값만 있을 경우)
  if (qFuzzyCompare(mn, mx)) {
    mx = mn + 1.0;
  }
  minValue_ = mn;
  maxValue_ = mx;
}

QVariantList DeviceGraphItem::normalizedValues() const {
  QVariantList result;
  const double range = maxValue_ - minValue_;
  if (qFuzzyIsNull(range))
    return result;

  for (const auto &v : qAsConst(history_)) {
    double val = 0.0;
    if (v.canConvert<QVariantMap>()) {
      auto map = v.toMap();
      if (map.contains("cpu"))
        val = map["cpu"].toDouble();
      else if (map.contains("memory"))
        val = map["memory"].toDouble();
      else if (map.contains("temp"))
        val = map["temp"].toDouble();
      else
        val = map.first().toDouble();
    } else {
      val = v.toDouble();
    }
    result.append((val - minValue_) / range);
  }
  return result;
}
