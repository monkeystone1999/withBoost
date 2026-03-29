#pragma once
#include <deque>
#include <map>
#include <string>
#include <array>
#include <mutex>
#include <cstdint>

/**
 * @file StatusManager.hpp
 * @brief 서버 상태 및 사용자 목록 관리자
 */

enum class UserState : uint8_t {
    Admin = 0,
    Normal,
    Pending
};

struct ServerStatus {
    std::deque<float> temp{20, 0.0f}, memory{20, 0.0f}, cpu{20, 0.0f};
    void Add(const std::array<float, 3>& meta);
};

struct UserMeta {
    std::string name_;
    UserState state_;
};

class StatusManager {
public:
    void AddUser(std::string name, std::string state);
    ServerStatus& getStatus() { return status_; }
    std::map<uint8_t, UserMeta>& getUsers() { return users_; }

private:
    ServerStatus status_;
    uint8_t nextUserId_ = 0;
    std::map<uint8_t, UserMeta> users_;
    std::map<std::string, UserState> nameToState_;
};
