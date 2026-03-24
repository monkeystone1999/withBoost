#include "Device.hpp"
#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>

// --- IStore Interface Implementation (DEPRECATED) ---
void DeviceStore::updateFromJson(const std::string &json, Callback cb) {
  // DEPRECATED: Use updateFromCameraJson or updateFromAvailableJson instead.
  // Core explicitly calls those methods based on the packet type.
  (void)json;
  if (cb)
    cb(snapshot());
}

// --- CAMERA packet processing (Static info update) ---
void DeviceStore::updateFromCameraJson(const std::string &json, Callback cb) {
  auto parsed = nlohmann::json::parse(json, nullptr, false);
  if (!parsed.is_array())
    return;

  std::vector<DeviceIntegrated> merged;

  {
    std::lock_guard<std::mutex> lk(mutex_);

    for (const auto &item : parsed) {
      if (!item.is_object())
        continue;

      std::string ip = item.value("ip", "");
      if (ip.empty())
        continue;

      // Find existing device by IP
      auto it =
          std::find_if(devices_.begin(), devices_.end(),
                       [&](const DeviceIntegrated &d) { return d.ip == ip; });

      DeviceIntegrated device;
      if (it != devices_.end()) {
        // Keep dynamic AVAILABLE info if exists
        device = *it;
      } else {
        // New device
        device.ip = ip;
      }

      // Update CAMERA info — parse cameraId in IP/streamNum format
      std::string sourceUrl = item.value("source_url", "");
      std::string cameraId;
      if (!sourceUrl.empty()) {
        size_t pos = sourceUrl.find(ip);
        if (pos != std::string::npos) {
          size_t slashPos = sourceUrl.find('/', pos + ip.length());
          if (slashPos != std::string::npos &&
              slashPos + 1 < sourceUrl.length()) {
            size_t nextSlash = sourceUrl.find('/', slashPos + 1);
            if (nextSlash != std::string::npos) {
              std::string streamNum =
                  sourceUrl.substr(slashPos + 1, nextSlash - slashPos - 1);
              cameraId = ip + "/" + streamNum;
            }
          }
        }
      }
      if (cameraId.empty()) {
        cameraId = ip + "/0"; // fallback
      }
      device.cameraId = cameraId;
      device.type = item.value("type", "");
      device.isOnline = item.value("is_online", false);

      // Capabilities (Currently only SUB_PI has motor)
      device.hasMotor = (device.type == "SUB_PI");
      device.hasIr = false;
      device.hasHeater = false;

      merged.push_back(device);
    }

    // Set devices offline if they are not in the current CAMERA list
    for (auto &old : devices_) {
      auto it = std::find_if(
          merged.begin(), merged.end(),
          [&](const DeviceIntegrated &d) { return d.ip == old.ip; });
      if (it == merged.end()) {
        old.isOnline = false;
        merged.push_back(old);
      }
    }

    devices_ = merged;
  } // lock released here — snapshot() must be called outside the lock

  if (cb)
    cb(std::move(merged)); // pass the already-built copy; avoids re-locking
}

// --- AVAILABLE packet processing (Dynamic info + History) ---
void DeviceStore::updateFromAvailableJson(const std::string &json,
                                          Callback cb) {
  auto parsed = nlohmann::json::parse(json, nullptr, false);
  if (!parsed.is_object())
    return;

  // Current timestamp in MS
  const auto now = std::chrono::system_clock::now();
  const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch())
                             .count();

  std::vector<DeviceIntegrated> snap;
  {
    std::lock_guard<std::mutex> lk(mutex_);

    if (parsed.contains("devices") && parsed["devices"].is_array()) {
      for (const auto &item : parsed["devices"]) {
        if (!item.is_object())
          continue;

        std::string ip = item.value("ip", "");
        if (ip.empty())
          continue;

        // Find existing device
        int idx = findIndexByIp(ip);
        DeviceIntegrated *device = nullptr;

        if (idx >= 0) {
          device = &devices_[idx];
        } else {
          // Temporary new device entry if CAMERA packet arrives later
          DeviceIntegrated newDev;
          newDev.ip = ip;
          devices_.push_back(newDev);
          device = &devices_.back();
        }

        // Update Dynamic info
        device->cpu = item.value("cpu", 0.0);
        device->memory = item.value("memory", 0.0);
        device->temp = item.value("temp", 0.0);
        device->uptime = item.value("uptime", 0);
        device->lastUpdate = timestamp;

        // Add Snapshot to history
        DeviceSnapshot s;
        s.timestamp = timestamp;
        s.cpu = device->cpu;
        s.memory = device->memory;
        s.temp = device->temp;
        s.uptime = device->uptime;

        addSnapshot(*device, s);
      }
    }

    snap = devices_; // copy while holding lock
  } // lock released here — callback must NOT hold the lock

  if (cb)
    cb(std::move(snap));
}

void DeviceStore::addSnapshot(DeviceIntegrated &device,
                              const DeviceSnapshot &snap) {
  device.history.push_back(snap);

  // Keep max 20 entries
  while (device.history.size() > MAX_HISTORY) {
    device.history.pop_front();
  }
}

std::vector<DeviceIntegrated> DeviceStore::snapshot() const {
  std::lock_guard<std::mutex> lk(mutex_);
  return devices_;
}

int DeviceStore::findIndexByIp(const std::string &ip) const {
  for (size_t i = 0; i < devices_.size(); ++i) {
    if (devices_[i].ip == ip)
      return static_cast<int>(i);
  }
  return -1;
}

bool DeviceStore::hasDevice(const std::string &ip) const {
  std::lock_guard<std::mutex> lk(mutex_);
  return findIndexByIp(ip) >= 0;
}

std::string DeviceStore::cameraIdByIp(const std::string &ip) const {
  std::lock_guard<std::mutex> lk(mutex_);
  int idx = findIndexByIp(ip);
  return idx >= 0 ? devices_[idx].cameraId : "";
}

std::vector<DeviceSnapshot>
DeviceStore::getHistory(const std::string &ip) const {
  std::lock_guard<std::mutex> lk(mutex_);
  int idx = findIndexByIp(ip);
  if (idx < 0)
    return {};

  // Convert deque to vector for returning
  const auto &hist = devices_[idx].history;
  return std::vector<DeviceSnapshot>(hist.begin(), hist.end());
}

void DeviceStore::clear() {
  std::lock_guard<std::mutex> lk(mutex_);
  devices_.clear();
}
