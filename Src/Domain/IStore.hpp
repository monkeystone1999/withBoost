#pragma once

#include <functional>
#include <mutex>
#include <string>


template <typename T> class IStore {
public:
  using Callback = std::function<void(T)>;
  virtual ~IStore() = default;
  virtual void updateFromJson(const std::string &json, Callback cb) = 0;
  virtual T snapshot() const = 0;
};
