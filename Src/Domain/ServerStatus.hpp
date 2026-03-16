#pragma once
#include "IStore.hpp"
#include <mutex>
#include <string>
#include <vector>

// ============================================================
//  ServerStatus Domain
// ============================================================

struct DeviceStatusData {
  std::string ip;
  double cpu = 0.0;
  double memory = 0.0;
  double temp = 0.0;
  int uptime = 0;
  int pendingEvents = 0;
};

struct ServerStatusData {
  double cpu = 0.0;
  double memory = 0.0;
  double temp = 0.0;
  int uptime = 0;
  bool available = false; // true only when "server" key present (admin)
  long long timestamp = 0;

  std::vector<DeviceStatusData> devices;
};

class ServerStatusStore : public IStore<ServerStatusData> {
public:
  void updateFromJson(const std::string &json, Callback cb) override;
  ServerStatusData snapshot() const override;

  std::vector<ServerStatusData> getHistory() const;

private:
  mutable std::mutex mutex_;
  ServerStatusData status_;
  std::vector<ServerStatusData> history_;
};
