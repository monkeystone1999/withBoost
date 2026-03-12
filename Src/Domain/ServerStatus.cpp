#include "ServerStatus.hpp"
#include <nlohmann/json.hpp>

void ServerStatusStore::updateFromJson(const std::string &json, Callback cb) {
  auto parsed = nlohmann::json::parse(json, nullptr, false);
  if (!parsed.is_object())
    return;

  ServerStatusData s;

  // Parse devices array
  if (parsed.contains("devices") && parsed["devices"].is_array()) {
    for (const auto &item : parsed["devices"]) {
      DeviceStatusData d;
      d.ip = item.value("ip", "");
      d.cpu = item.value("cpu", 0.0);
      d.memory = item.value("memory", 0.0);
      d.temp = item.value("temp", 0.0);
      d.uptime = item.value("uptime", 0);
      d.pendingEvents = item.value("pending_events", 0);
      s.devices.push_back(std::move(d));
    }
  }

  // "server" key is admin-only
  if (parsed.contains("server") && parsed["server"].is_object()) {
    const auto &srv = parsed["server"];
    s.cpu = srv.value("cpu", 0.0);
    s.memory = srv.value("memory", 0.0);
    s.temp = srv.value("temp", 0.0);
    s.uptime = srv.value("uptime", 0);
    s.available = true;
  }

  {
    std::lock_guard<std::mutex> lk(mutex_);
    status_ = s;
  }

  if (cb)
    cb(std::move(s));
}

ServerStatusData ServerStatusStore::snapshot() const {
  std::lock_guard<std::mutex> lk(mutex_);
  return status_;
}

double ServerStatusStore::deviceCpu(const std::string &ip) const {
  std::lock_guard<std::mutex> lk(mutex_);
  for (auto &d : status_.devices)
    if (d.ip == ip)
      return d.cpu;
  return 0.0;
}
double ServerStatusStore::deviceMemory(const std::string &ip) const {
  std::lock_guard<std::mutex> lk(mutex_);
  for (auto &d : status_.devices)
    if (d.ip == ip)
      return d.memory;
  return 0.0;
}
double ServerStatusStore::deviceTemp(const std::string &ip) const {
  std::lock_guard<std::mutex> lk(mutex_);
  for (auto &d : status_.devices)
    if (d.ip == ip)
      return d.temp;
  return 0.0;
}
int ServerStatusStore::deviceUptime(const std::string &ip) const {
  std::lock_guard<std::mutex> lk(mutex_);
  for (auto &d : status_.devices)
    if (d.ip == ip)
      return d.uptime;
  return 0;
}
