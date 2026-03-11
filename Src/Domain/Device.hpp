#pragma once
#include "IStore.hpp"
#include <mutex>
#include <string>
#include <vector>


// ============================================================
//  Device Domain
// ============================================================

struct DeviceCapabilityData {
  bool motor = true;
  bool ir = false;
  bool heater = false;
};

struct DeviceData {
  std::string rtspUrl;
  std::string deviceIp;
  std::string title;
  bool isOnline = false;
  DeviceCapabilityData cap;
};

class DeviceStore : public IStore<std::vector<DeviceData>> {
public:
  void updateFromJson(const std::string &json, Callback cb) override;
  std::vector<DeviceData> snapshot() const override;

  int findIndex(const std::string &rtspUrl) const;
  bool hasDevice(const std::string &rtspUrl) const;
  bool hasMotor(const std::string &rtspUrl) const;
  std::string deviceIp(const std::string &rtspUrl) const;

private:
  mutable std::mutex m_mutex;
  std::vector<DeviceData> m_data;
};
