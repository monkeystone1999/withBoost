#include "ServerStatus.hpp"

ServerStatusModel::ServerStatusModel(QObject *parent) : QObject(parent) {}

void ServerStatusModel::refreshFromStatusManager() {
  if (!statusManager_)
    return;
  emit statusUpdated();
}

QVariantList ServerStatusModel::getServerHistory() const {
  if (!statusManager_)
    return {};

  auto &stat = statusManager_->getStatus();
  size_t count = stat.cpu.size();

  QVariantList list;
  for (size_t i = 0; i < count; ++i) {
    QVariantMap point;
    point["cpu"] = static_cast<double>(stat.cpu[i]);
    point["memory"] = static_cast<double>(stat.memory[i]);
    point["temp"] = static_cast<double>(stat.temp[i]);
    list.append(point);
  }
  return list;
}

double ServerStatusModel::deviceCpu(const QString &) const { return 0.0; }
double ServerStatusModel::deviceMemory(const QString &) const { return 0.0; }
double ServerStatusModel::deviceTemp(const QString &) const { return 0.0; }
int ServerStatusModel::deviceUptime(const QString &) const { return 0; }
