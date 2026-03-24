#pragma once
#include "IStore.hpp"
#include <map>
#include <mutex>
#include <string>
#include <vector>

// ============================================================
//  Camera Domain
// ============================================================

struct DeviceInfo {
  double hum = 0.0;
  double light = 0.0;
  double tilt = 0.0;
  double tmp = 0.0;
};

struct CameraData {
  std::string title;
  std::string cameraType; // "HANWHA" | "SUB_PI"
  bool isOnline = false;
  std::string cameraId; // For looking up CameraStruct
  DeviceInfo deviceInfo;
};

struct AiInfo {
  std::vector<std::string> Img;
  std::vector<int> personCount;
};

struct CameraStruct {
  DeviceInfo deviceInfo;
  AiInfo aiInfo;
  std::string sourceUrl;
};

enum class SplitDirection {
  None = 0, // splitCount == 1, no split
  Col = 1,  // X-axis split (Col-2, Col-3, Col-4)
  Row = 2,  // Y-axis split (Row-2, Row-3)
  Grid = 3, // 2D grid (Grid-4)
};

class CameraStore : public IStore<std::vector<CameraData>> {
public:
  void updateFromJson(const std::string &json, Callback cb) override;
  std::vector<CameraData> snapshot() const override;

  // New thread-safe API for accessing CameraStruct data
  void updateDeviceInfo(const std::string &cameraId, const DeviceInfo &info);
  void updateAiInfo(const std::string &cameraId, const AiInfo &info);
  CameraStruct getCameraStruct(const std::string &cameraId) const;
  void clear();

private:
  mutable std::mutex mutex_;
  std::vector<CameraData> data_;
  std::map<std::string, CameraStruct> table_;
};
