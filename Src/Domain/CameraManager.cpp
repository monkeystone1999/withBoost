#include "CameraManager.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

// ─────────────────────────────────────────────────────────────
// CameraMeta Implementation
// ─────────────────────────────────────────────────────────────

void CameraMeta::Add(const std::array<float, 4>& meta, const std::string& str) {
    dir = str;
    tmp_.pop_front();
    tmp_.push_back(meta[0]);
    tilt_.pop_front();
    tilt_.push_back(meta[1]);
    light_.pop_front();
    light_.push_back(meta[2]);
    hum_.pop_front();
    hum_.push_back(meta[3]);
}

// ─────────────────────────────────────────────────────────────
// CameraImage Implementation
// ─────────────────────────────────────────────────────────────

void CameraImage::SetMeta(int jpeg_size, int total_frames) {
    jpeg_size_ = jpeg_size;
    total_frames_ = total_frames;
}

void CameraImage::Add(IMG img) {
    if (ImageList_.size() >= 20) {
        ImageList_.pop_front();
    }
    ImageList_.push_back(std::move(img));
}

void CameraImage::Feed(const ImageHeader& header, const uint8_t* imageData, size_t imageSize) {
    auto& slot = pendingFrames_[header.FrameNumber];
    slot.maxSequence = header.MaxSequenceNumber;
    slot.fragments[header.SequenceNumber].assign(imageData, imageData + imageSize);
}

bool CameraImage::isFrameComplete(uint8_t frameNumber) const {
    auto it = pendingFrames_.find(frameNumber);
    if (it == pendingFrames_.end()) return false;
    return static_cast<uint8_t>(it->second.fragments.size()) == static_cast<uint8_t>(it->second.maxSequence + 1);
}

CameraImage::IMG CameraImage::ExtractFrame(uint8_t frameNumber) {
    IMG result;
    auto it = pendingFrames_.find(frameNumber);
    if (it == pendingFrames_.end()) return result;

    auto& slot = it->second;
    for (uint8_t seq = 0; seq <= slot.maxSequence; ++seq) {
        auto fIt = slot.fragments.find(seq);
        if (fIt != slot.fragments.end()) {
            result.insert(result.end(), fIt->second.begin(), fIt->second.end());
        }
    }
    pendingFrames_.erase(it);
    Add(result);
    return result;
}

std::string CameraImage::BuildResultJson(uint8_t frameNumber) const {
    nlohmann::json j;
    auto it = pendingFrames_.find(frameNumber);
    if (it == pendingFrames_.end() || isFrameComplete(frameNumber)) { 
        j["receive"] = true; 
        return j.dump(); 
    }
    
    nlohmann::json lossArr = nlohmann::json::array();
    for (uint8_t seq = 0; seq <= it->second.maxSequence; ++seq) {
        if (it->second.fragments.find(seq) == it->second.fragments.end()) {
            nlohmann::json missing;
            missing["frame"] = frameNumber;
            missing["sequence"] = seq;
            lossArr.push_back(missing);
        }
    }
    j["loss"] = lossArr;
    return j.dump();
}

void CameraImage::Cleanup(uint8_t currentFrame) {
    for (auto it = pendingFrames_.begin(); it != pendingFrames_.end();) {
        uint8_t diff = currentFrame - it->first;
        if (diff > 128) it = pendingFrames_.erase(it);
        else ++it;
    }
}

// ─────────────────────────────────────────────────────────────
// CameraManager Implementation
// ─────────────────────────────────────────────────────────────

uint32_t CameraManager::Add(std::string ip, std::string rtsp) {
    auto& indices = ipTable_[ip];
    uint8_t nextIndex = 0;
    while (indices.count(nextIndex)) { nextIndex++; }
    indices.insert(nextIndex);
    std::string networkId = ip + "/" + std::to_string(nextIndex);
    uint32_t virtualId = Id_++;
    IpToId_[networkId] = virtualId;
    map_[virtualId] = { .rtsp_ = rtsp, .ip_ = ip, .index_ = nextIndex };
    return virtualId;
}

void CameraManager::Remove(uint32_t id) {
    auto it = map_.find(id);
    if (it == map_.end()) return;
    const auto& info = it->second;
    std::string networkId = info.ip_ + "/" + std::to_string(info.index_);
    ipTable_[info.ip_].erase(info.index_);
    if (ipTable_[info.ip_].empty()) ipTable_.erase(info.ip_);
    IpToId_.erase(networkId);
    map_.erase(it);
}

CameraInfo* CameraManager::Get(uint32_t id) {
    auto it = map_.find(id);
    return (it != map_.end()) ? &it->second : nullptr;
}

CameraInfo* CameraManager::Get(const std::string& networkId) {
    auto it = IpToId_.find(networkId);
    return (it != IpToId_.end()) ? Get(it->second) : nullptr;
}

CameraInfo* CameraManager::GetByIp(const std::string& ip) {
    auto it = IpToId_.find(ip);
    return (it != IpToId_.end()) ? Get(it->second) : nullptr;
}

std::vector<uint32_t> CameraManager::GetIdsByIp(const std::string& ip) {
    std::vector<uint32_t> result;
    for (auto& [id, info] : map_) {
        if (info.ip_ == ip) result.push_back(id);
    }
    return result;
}
