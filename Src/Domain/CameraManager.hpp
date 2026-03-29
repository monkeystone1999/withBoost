#pragma once
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include "../Network/NetworkProtocol.hpp"

/**
 * @file CameraManager.hpp
 * @brief 카메라 데이터 개체 및 중앙 관리자
 */

#include "../Network/VideoEngine.hpp"

struct CameraMeta {
    std::deque<float> tmp_{20, 0.0f}, tilt_{20, 0.0f}, light_{20, 0.0f}, hum_{20, 0.0f};
    std::string dir{""};
    void Add(const std::array<float, 4>& meta, const std::string& str);
};

struct CameraStatus {
    bool motor_auto{false};
    bool ir_on{false};
    bool heater_on{false};
};

struct CameraImage {
    using IMG = std::vector<uint8_t>;
    std::deque<IMG> ImageList_;
    int jpeg_size_ = 0;
    int total_frames_ = 0;

    struct FrameSlot {
        uint8_t maxSequence = 0;
        std::map<uint8_t, std::vector<uint8_t>> fragments;
    };
    std::map<uint8_t, FrameSlot> pendingFrames_;

    void SetMeta(int jpeg_size, int total_frames);
    void Add(IMG img);
    void Feed(const ImageHeader& header, const uint8_t* imageData, size_t imageSize);
    bool isFrameComplete(uint8_t frameNumber) const;
    IMG ExtractFrame(uint8_t frameNumber);
    std::string BuildResultJson(uint8_t frameNumber) const;
    void Cleanup(uint8_t currentFrame);
};

struct CameraInfo {
    std::string rtsp_{""}, ip_{""};
    uint8_t index_{0};
    CameraMeta MetaData_;
    CameraImage Images_;
    CameraStatus Status_;
    std::unique_ptr<VideoEngine> video_; 
};

class CameraManager {
public:
    CameraManager() = default;
    ~CameraManager() = default;

    uint32_t Add(std::string ip, std::string rtsp);
    void Remove(uint32_t id);
    
    std::map<uint32_t, CameraInfo>& getCameras() { return map_; }
    CameraInfo* Get(uint32_t id);
    CameraInfo* Get(const std::string& networkId);
    CameraInfo* GetByIp(const std::string& ip);
    std::vector<uint32_t> GetIdsByIp(const std::string& ip);

private:
    std::map<uint32_t, CameraInfo> map_;
    std::map<std::string, uint32_t> IpToId_;
    std::map<std::string, std::set<uint8_t>> ipTable_;
    uint32_t Id_ = 0;
};
