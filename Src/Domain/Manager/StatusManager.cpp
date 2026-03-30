#include "StatusManager.hpp"

void ServerStatus::Add(const std::array<float, 3> &meta) {
  temp.pop_front();
  temp.push_back(meta[0]);
  memory.pop_front();
  memory.push_back(meta[1]);
  cpu.pop_front();
  cpu.push_back(meta[2]);
}

void StatusManager::AddUser(std::string name, std::string state) {
  if (nameToState_.find(name) != nameToState_.end())
    return;

  UserState userState = UserState::Pending;
  if (state == "Admin")
    userState = UserState::Admin;
  else if (state == "Normal")
    userState = UserState::Normal;

  nameToState_[name] = userState;
  users_[nextUserId_++] = {.name_ = name, .state_ = userState};
}

/**
 * @section Workflow Guide
 *
 * **[StatusManager 구현 세부 사항]**
 *
 * 1. 서버 자원 데이터 누적:
 *    - `ServerStatus::Add`는 고정 크기(20)를 유지하기 위해 `pop_front` 후
 * `push_back`을 수행합니다.
 *    - 이는 이동 평균(Moving Average) 계산이나 롤링 차트 UI 구현에 최적화된
 * 구조입니다.
 *
 * 2. 유저 관리 식별:
 *    - 중복 로그인을 방지하기 위해 `nameToState_` 맵을 사용하여 이름 기반
 * 조회를 우선 수행합니다.
 *    - 유동적인 유저 관리를 위해 내부적으로 자동 증가 ID(`nextUserId_`)를
 * 부여합니다.
 */
