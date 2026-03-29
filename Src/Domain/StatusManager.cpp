#include "StatusManager.hpp"

void ServerStatus::Add(const std::array<float, 3>& meta) {
    temp.pop_front();
    temp.push_back(meta[0]);
    memory.pop_front();
    memory.push_back(meta[1]);
    cpu.pop_front();
    cpu.push_back(meta[2]);
}

void StatusManager::AddUser(std::string name, std::string state) {
    if (nameToState_.find(name) != nameToState_.end()) return;

    UserState userState = UserState::Pending;
    if (state == "Admin") userState = UserState::Admin;
    else if (state == "Normal") userState = UserState::Normal;

    nameToState_[name] = userState;
    users_[nextUserId_++] = { .name_ = name, .state_ = userState };
}
