#include "Camera.hpp"
#include <nlohmann/json.hpp>

void CameraStore::updateFromJson(const std::string &json, Callback cb) {
  auto parsed =
      nlohmann::json::parse(json, nullptr, /*allow_exceptions=*/false);
  if (!parsed.is_array())
    return;

  std::vector<CameraData> newData;
  newData.reserve(parsed.size());

  for (const auto &item : parsed) {
    if (!item.is_object())
      continue;
    CameraData d;
    std::string sourceUrl = item.value("source_url", "");
    d.cameraType = item.value("type", "");
    d.isOnline = item.value("is_online", false);
    const std::string ip = item.value("ip", "");
    d.title = ip + " (" + d.cameraType + ")";

    std::string cameraId;
    if (!sourceUrl.empty()) {
      // Extract IP and port from sourceUrl
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
    if (cameraId.empty() && !ip.empty()) {
      cameraId = ip + "/0"; // fallback
    }

    if (!cameraId.empty()) {
      d.cameraId = cameraId;
      newData.push_back(d);
      std::lock_guard<std::mutex> lk(mutex_);
      if (table_.find(cameraId) == table_.end()) {
        table_[cameraId] = CameraStruct{};
      }
      table_[cameraId].sourceUrl = sourceUrl;
    }
  }

  {
    std::lock_guard<std::mutex> lk(mutex_);
    data_ = newData;
  }

  if (cb)
    cb(std::move(newData));
}

std::vector<CameraData> CameraStore::snapshot() const {
  std::lock_guard<std::mutex> lk(mutex_);
  std::vector<CameraData> result = data_;
  for (auto &d : result) {
    auto it = table_.find(d.cameraId);
    if (it != table_.end()) {
      d.deviceInfo = it->second.deviceInfo;
    }
  }
  return result;
}

void CameraStore::updateDeviceInfo(const std::string &cameraId,
                                   const DeviceInfo &info) {
  std::lock_guard<std::mutex> lk(mutex_);
  table_[cameraId].deviceInfo = info;
}

void CameraStore::updateAiInfo(const std::string &cameraId,
                               const AiInfo &info) {
  std::lock_guard<std::mutex> lk(mutex_);
  table_[cameraId].aiInfo = info;
}

CameraStruct CameraStore::getCameraStruct(const std::string &cameraId) const {
  std::lock_guard<std::mutex> lk(mutex_);
  auto it = table_.find(cameraId);
  if (it != table_.end()) {
    return it->second;
  }
  return CameraStruct{};
}

void CameraStore::clear() {
  std::lock_guard<std::mutex> lk(mutex_);
  data_.clear();
  table_.clear();
}
