#pragma once
#include <chrono>
#include <string>


enum class UserRole { User, Admin, Pending };

struct UserData {
  std::string userId;
  std::string username;
  std::string email;
  UserRole role;
  bool isOnline;
  std::chrono::system_clock::time_point lastLogin;
  std::string ipAddress;
  int activeCameras;

  UserData() : role(UserRole::User), isOnline(false), activeCameras(0) {}
};
