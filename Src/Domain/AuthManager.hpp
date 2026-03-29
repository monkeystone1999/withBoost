#pragma once
#include <string>
#include <chrono>

/**
 * @file AuthManager.hpp
 * @brief 사용자 인증 정보 및 권한 관리자
 */

enum class UserRole {
    User,
    Admin
};

struct UserData {
    std::string userId;
    std::string username;
    std::string email;
    UserRole role;
    bool isOnline;
    std::chrono::system_clock::time_point lastLogin;
    std::string ipAddress;
    int activeCameras;
    
    UserData() 
        : role(UserRole::User)
        , isOnline(false)
        , activeCameras(0) {}
};

class AuthManager {
public:
    void Login(const std::string& username, const std::string& password) {
        // ... Login logic ...
    }
    
    void Logout() {
        currentUser_ = UserData();
    }

    const UserData& getCurrentUser() const { return currentUser_; }

private:
    UserData currentUser_;
};
