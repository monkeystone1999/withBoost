#include "DeviceStore.hpp"
#include <nlohmann/json.hpp>

void DeviceStore::updateFromJson(const std::string &json, Callback cb) {
    auto parsed = nlohmann::json::parse(json, nullptr, false);
    if (!parsed.is_array()) return;

    std::vector<DeviceData> newData;

    for (const auto &item : parsed) {
        if (!item.is_object()) continue;
        // Only SUB_PI cameras are controllable devices
        if (item.value("type", "") != "SUB_PI") continue;

        DeviceData d;
        d.rtspUrl  = item.value("source_url", "");
        d.deviceIp = item.value("ip",         "");
        d.title    = d.deviceIp + " (SUB_PI)";
        d.isOnline = item.value("is_online",  false);
        d.cap.motor  = true;
        d.cap.ir     = false;
        d.cap.heater = false;

        if (!d.rtspUrl.empty())
            newData.push_back(std::move(d));
    }

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_data = newData;
    }

    if (cb) cb(std::move(newData));
}

std::vector<DeviceData> DeviceStore::snapshot() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_data;
}

int DeviceStore::findIndex(const std::string &rtspUrl) const {
    for (int i = 0; i < (int)m_data.size(); ++i)
        if (m_data[i].rtspUrl == rtspUrl) return i;
    return -1;
}

bool DeviceStore::hasDevice(const std::string &rtspUrl) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return findIndex(rtspUrl) >= 0;
}

bool DeviceStore::hasMotor(const std::string &rtspUrl) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    int i = findIndex(rtspUrl);
    return i >= 0 ? m_data[i].cap.motor : false;
}

std::string DeviceStore::deviceIp(const std::string &rtspUrl) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    int i = findIndex(rtspUrl);
    return i >= 0 ? m_data[i].deviceIp : "";
}
