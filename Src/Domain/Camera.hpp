#pragma once
#include "IStore.hpp"
#include <mutex>
#include <string>
#include <vector>


// ============================================================
//  Camera Domain
// ============================================================

struct CameraData {
  std::string title;
  std::string rtspUrl;
  std::string cameraType; // "HANWHA" | "SUB_PI"
  bool isOnline = false;
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

private:
  mutable std::mutex m_mutex;
  std::vector<CameraData> m_data;
};
