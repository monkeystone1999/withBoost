#include "CameraStore.hpp"
#include <nlohmann/json.hpp>

void CameraStore::updateFromJson(const std::string &json, Callback cb) {
    auto parsed = nlohmann::json::parse(json, nullptr, /*allow_exceptions=*/false);
    if (!parsed.is_array()) return;

    std::vector<CameraData> newData;
    newData.reserve(parsed.size());

    for (const auto &item : parsed) {
        if (!item.is_object()) continue;
        CameraData d;
        d.rtspUrl    = item.value("source_url", "");
        d.cameraType = item.value("type",        "");
        d.isOnline   = item.value("is_online",   false);
        const std::string ip = item.value("ip",  "");
        d.title = ip + " (" + d.cameraType + ")";
        if (!d.rtspUrl.empty())
            newData.push_back(std::move(d));
    }

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_data = newData;
    }

    if (cb) cb(std::move(newData));
}

std::vector<CameraData> CameraStore::snapshot() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_data;
}
