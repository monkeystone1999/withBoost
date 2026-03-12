#pragma once
#include "IStore.hpp"
#include <deque>
#include <mutex>
#include <string>
#include <vector>

// --- Device Snapshot (History Entry) ---
struct DeviceSnapshot {
  int64_t timestamp; // Unix time (ms)
  double cpu;
  double memory;
  double temp;
  int uptime;
};

// --- Integrated Device Object (IP-based) ---
struct DeviceIntegrated {
  // Primary Key
  std::string ip;

  // From CAMERA packet (static)
  std::string rtspUrl;
  std::string type; // "SUB_PI", "JETSON", etc.
  bool isOnline = false;

  // From AVAILABLE packet (dynamic, 5s update)
  double cpu = 0.0;
  double memory = 0.0;
  double temp = 0.0;
  int uptime = 0;
  int64_t lastUpdate = 0; // Unix timestamp (ms)

  // Capabilities
  bool hasMotor = true;
  bool hasIr = false;
  bool hasHeater = false;

  // History (Max 20 entries, FIFO)
  std::deque<DeviceSnapshot> history;
};

// --- DeviceStore (IP-based integrated storage) ---
class DeviceStore : public IStore<std::vector<DeviceIntegrated>> {
public:
  // --- IStore Interface Implementation ---
  // DEPRECATED: updateFromJson is not used.
  // CAMERA and AVAILABLE are handled by separate methods.
  void updateFromJson(const std::string &json, Callback cb) override;

  // CAMERA packet processing (static info update)
  void updateFromCameraJson(const std::string &json, Callback cb);

  // AVAILABLE packet processing (dynamic info + history update)
  void updateFromAvailableJson(const std::string &json, Callback cb);

  std::vector<DeviceIntegrated> snapshot() const override;

  // IP-based lookup
  int findIndexByIp(const std::string &ip) const;
  bool hasDevice(const std::string &ip) const;
  std::string rtspUrlByIp(const std::string &ip) const;

  // History lookup (max 20 entries)
  std::vector<DeviceSnapshot> getHistory(const std::string &ip) const;

private:
  void addSnapshot(DeviceIntegrated &device, const DeviceSnapshot &snap);

  mutable std::mutex mutex_;
  std::vector<DeviceIntegrated> devices_;

  static constexpr size_t MAX_HISTORY = 20;
};
