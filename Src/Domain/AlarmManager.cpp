#include "AlarmManager.hpp"
#include <nlohmann/json.hpp>

void AlarmManager::dispatch(const std::string &json, Callback cb) {
    if (!cb) return;
    pool_.Submit([json, cb = std::move(cb)]() {
        auto parsed = nlohmann::json::parse(json, nullptr, false);
        if (!parsed.is_object() || parsed.value("type", "") != "alarm") return;

        AlarmEvent ev;
        ev.title = parsed.value("title", "AI Alert");
        ev.detail = parsed.value("detail", "Anomaly detected");
        ev.severity = parsed.value("severity", 1);
        cb(ev);
    });
}
