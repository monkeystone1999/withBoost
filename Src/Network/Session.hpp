#pragma once

#include "Header.hpp"

#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// Build json object from key/value pairs.
template <typename... Args>
nlohmann::json to_json(const std::pair<std::string, Args> &...pairs) {
  nlohmann::json j = nlohmann::json::object();
  (j.emplace(pairs.first, pairs.second), ...);
  return j;
}

// Build wire packet: [PacketHeader | json body bytes]
inline std::vector<uint8_t> MakePacket(MessageType type,
                                       const nlohmann::json &body) {
  std::string json_str = body.dump();
  std::vector<uint8_t> packet(sizeof(PacketHeader) + json_str.size());

  PacketHeader hdr;
  hdr.type = type;
  hdr.body_length = static_cast<uint32_t>(json_str.size());

  std::memcpy(packet.data(), &hdr, sizeof(PacketHeader));
  std::memcpy(packet.data() + sizeof(PacketHeader), json_str.data(),
              json_str.size());
  return packet;
}

class ConnectServer : public std::enable_shared_from_this<ConnectServer> {
public:
  std::function<void()> onConnected;
  std::function<void(const std::string &)> onCameraMessage;
  std::function<void(const std::string &)> onAiMessage;
  std::function<void(const std::string &)> onSuccess;
  std::function<void(const std::string &)> onFail;
  std::function<void(const std::string &)> onAvailable;
  std::function<void(const std::string &)> onAssignMessage;
  std::function<void(const std::string &)> onDeviceMessage;

  ConnectServer(boost::asio::io_context &io, const std::string ip,
                const std::string port)
      : io_(io), socket_(io), strand_(boost::asio::make_strand(io)) {
    boost::asio::ip::tcp::resolver resolver(io);
    endpoint_ = resolver.resolve(ip, port);
    Message_.reserve(4096);
  }

  void StartConnect() { Connect(endpoint_); }

  void WriteRaw(MessageType type, const std::string &json_str) {
    std::vector<uint8_t> packet(sizeof(PacketHeader) + json_str.size());
    PacketHeader hdr;
    hdr.type = type;
    hdr.body_length = static_cast<uint32_t>(json_str.size());

    std::memcpy(packet.data(), &hdr, sizeof(PacketHeader));
    std::memcpy(packet.data() + sizeof(PacketHeader), json_str.data(),
                json_str.size());

    boost::asio::post(strand_, [this, pkt = std::move(packet)]() mutable {
      const bool idle = write_queue_.empty();
      write_queue_.push_back(std::move(pkt));
      if (idle)
        DoWrite();
    });
  }

  template <typename... Arg>
  void Write(MessageType type, std::pair<std::string, Arg> &&...pairs) {
    nlohmann::json body = to_json(pairs...);
    auto packet = MakePacket(type, body);

    boost::asio::post(strand_, [this, pkt = std::move(packet)]() mutable {
      const bool idle = write_queue_.empty();
      write_queue_.push_back(std::move(pkt));
      if (idle)
        DoWrite();
    });
  }

  void Close() {
    auto self(shared_from_this());
    boost::asio::post(strand_, [this, self]() {
      boost::system::error_code ec;
      socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
      socket_.close(ec);
    });
  }

private:
  void Connect(const boost::asio::ip::tcp::resolver::results_type &endpoint) {
    boost::asio::async_connect(
        socket_, endpoint,
        boost::asio::bind_executor(
            strand_,
            [this](boost::system::error_code ec,
                   const boost::asio::ip::tcp::endpoint &) {
              if (!ec) {
                if (onConnected)
                  onConnected();
                ReadHeader();
              } else {
                HandleError(ec);
              }
            }));
  }

  void ReadHeader() {
    boost::asio::async_read(
        socket_, boost::asio::buffer(header_buf_.data(), sizeof(PacketHeader)),
        boost::asio::bind_executor(
            strand_, [this](boost::system::error_code ec, std::size_t) {
              if (ec) {
                HandleError(ec);
                return;
              }

              BasePacketHeader =
                  reinterpret_cast<PacketHeader *>(header_buf_.data());
              ReadBody(BasePacketHeader->body_length);
            }));
  }

  void ReadBody(uint32_t len) {
    if (len == 0) {
      Message_.clear();
      HeaderSwitch();
      ReadHeader();
      return;
    }

    Message_.resize(len);

    boost::asio::async_read(
        socket_, boost::asio::buffer(Message_.data(), len),
        boost::asio::bind_executor(
            strand_, [this](boost::system::error_code ec, std::size_t) {
              if (ec) {
                HandleError(ec);
                return;
              }

              HeaderSwitch();
              ReadHeader();
            }));
  }

  void HeaderSwitch() {
    switch (BasePacketHeader->type) {
    case MessageType::ACK:
      break;
    case MessageType::SUCCESS: {
      std::string json_str(Message_.begin(), Message_.end());
      if (onSuccess)
        onSuccess(json_str);
      break;
    }
    case MessageType::FAIL: {
      std::string json_str(Message_.begin(), Message_.end());
      if (onFail)
        onFail(json_str);
      break;
    }
    case MessageType::AVAILABLE: {
      std::string json_str(Message_.begin(), Message_.end());
      if (onAvailable)
        onAvailable(json_str);
      break;
    }
    case MessageType::AI: {
      std::string json_str(Message_.begin(), Message_.end());
      if (onAiMessage)
        onAiMessage(json_str);
      break;
    }
    case MessageType::CAMERA: {
      std::string json_str(Message_.begin(), Message_.end());
      if (onCameraMessage)
        onCameraMessage(json_str);
      break;
    }
    case MessageType::ASSIGN: {
      std::string json_str(Message_.begin(), Message_.end());
      if (onAssignMessage)
        onAssignMessage(json_str);
      break;
    }
    case MessageType::DEVICE: {
      std::string json_str(Message_.begin(), Message_.end());
      if (onDeviceMessage)
        onDeviceMessage(json_str);
      break;
    }
    default:
      break;
    }
  }

  void DoWrite() {
    boost::asio::async_write(
        socket_, boost::asio::buffer(write_queue_.front()),
        boost::asio::bind_executor(
            strand_, [this](boost::system::error_code ec, std::size_t) {
              if (ec) {
                HandleError(ec);
                return;
              }

              write_queue_.pop_front();
              if (!write_queue_.empty())
                DoWrite();
            }));
  }

  void HandleError(boost::system::error_code ec) {
    if (ec == boost::asio::error::operation_aborted)
      return;

    boost::system::error_code ignored;
    socket_.close(ignored);
  }

private:
  boost::asio::io_context &io_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  std::array<uint8_t, sizeof(PacketHeader)> header_buf_{};
  std::vector<uint8_t> Message_;
  std::deque<std::vector<uint8_t>> write_queue_;

  boost::asio::ip::tcp::resolver::results_type endpoint_;
  PacketHeader *BasePacketHeader = nullptr;
};

class SessionManager {
public:
  SessionManager() {
    work_guard_ =
        std::make_unique<WorkGuard>(boost::asio::make_work_guard(io_context_));
    io_thread_ = std::thread([this] { io_context_.run(); });
  }

  ~SessionManager() {
    work_guard_.reset();
    if (io_thread_.joinable())
      io_thread_.join();
  }

  void AddSession(const std::string &name, const std::string &ip,
                  const std::string &port) {
    manager_[name] = std::make_shared<ConnectServer>(io_context_, ip, port);
  }

  std::shared_ptr<ConnectServer> GetSession(const std::string &name) {
    auto it = manager_.find(name);
    return it != manager_.end() ? it->second : nullptr;
  }

  void CloseAll() {
    for (auto &pair : manager_) {
      if (pair.second)
        pair.second->Close();
    }
  }

  void RemoveSession(const std::string &name) {
    auto it = manager_.find(name);
    if (it != manager_.end()) {
      if (it->second)
        it->second->Close();
      manager_.erase(it);
    }
  }

private:
  using WorkGuard =
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

  boost::asio::io_context io_context_;
  std::unique_ptr<WorkGuard> work_guard_;
  std::thread io_thread_;
  std::map<std::string, std::shared_ptr<ConnectServer>> manager_;
};
