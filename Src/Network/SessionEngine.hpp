#ifndef SESSIONENGINE_HPP
#define SESSIONENGINE_HPP

#include "../Config.hpp"
#include "../Crypt/DTLS.hpp"
#include "../Crypt/TLS.hpp"
#include "../Thread/ThreadPool.hpp"
#include "Protocol.hpp"
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <sigslot/signal.hpp>

enum class Protocol { TCP, UDP };

using CameraPayload = std::variant<std::string,
                                   std::vector<uint8_t>,
                                   std::pair<std::vector<float>,std::string>>;

using Payload = std::shared_ptr<std::vector<uint8_t>>;

class NetworkManager {
public:
    sigslot::signal<std::string, CameraPayload> CameraBridge_;
    sigslot::signal<std::string> Auth_;
    NetworkManager(){}
    void Register(MessageType type, std::function<void(Payload)> callback){
        map_[type] = callback;
    }
    void Trigger(MessageType type, Payload Payload_){
        auto it = map_.find(type);
        if(it == map_.end()){
            /// Error 처리
        }else{
            map_[type](Payload_);
        }
    }
    boost::asio::io_context& getIO(){
        return io_;
    }
private:
    std::map<MessageType, std::function<void(Payload)>> map_;
    boost::asio::io_context io_;
};


template <Protocol P> class Session;


template <>
class Session<Protocol::TCP>
    : public std::enable_shared_from_this<Session<Protocol::TCP>> {
public:
  explicit Session(boost::asio::io_context &io)
      : io_(io), strand_(boost::asio::make_strand(io)), socket_(io) {};
  void Connect(const std::string ip, const uint16_t port) {
    boost::asio::ip::tcp::endpoint endpoint_(boost::asio::ip::make_address(ip),
                                             port);
    socket_.async_connect(
        endpoint_, boost::asio::bind_executor(
                       strand_, [this](const boost::system::error_code &ec) {
                         /// if(ec) {on_error_(ec); return;}
                         ///
                       }));
  }
  void Read(std::shared_ptr<std::vector<uint8_t>> data,
            std::function<void()> After) {
    boost::asio::async_read(
        socket_, boost::asio::buffer(data->data(), data->size()),
        boost::asio::bind_executor(
            strand_,
            [self = shared_from_this(), data = std::move(data),
             After = std::move(After)](const boost::system::error_code &ec,
                                       std::size_t bytes) { After(); }));
  }
  void Read(std::shared_ptr<std::vector<uint8_t>> data) {
    boost::asio::async_read(
        socket_, boost::asio::buffer(data->data(), data->size()),
        boost::asio::bind_executor(
            strand_,
            [self = shared_from_this(), data = std::move(data)](
                const boost::system::error_code &ec, std::size_t bytes) {}));
  }

  void Write(std::vector<uint8_t> data) {
    auto buf = std::make_shared<std::vector<uint8_t>>(std::move(data));
    boost::asio::async_write(
        socket_, boost::asio::buffer(*buf),
        boost::asio::bind_executor(
            strand_,
            [self = shared_from_this(),
             buf](const boost::system::error_code &ec, std::size_t bytes) {}));
  }

private:
  boost::asio::io_context &io_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
};

class ServerConnect
    : public std::enable_shared_from_this<
          ServerConnect> { /// decrypt, encrypt 그리고 Packet 처리
public:
  explicit ServerConnect(NetworkManager& Manager, std::string ip,
                         uint16_t port)
        : NetworkManager_(Manager),io_(Manager.getIO()), Session_(std::make_shared<Session<Protocol::TCP>>(Manager.getIO())) {
    Session_->Connect(ip, port);
  }
  void run(ThreadPool *pool = nullptr) {
    if (pool == nullptr) {
      io_.run();
    } else {
      pool_ = pool;
        pool_->Submit([self = shared_from_this()]() { self->run(); });
    }
  }
  void Read(){
      auto Header = std::make_shared<std::vector<uint8_t>>(sizeof(PacketHeader));
      Session_->Read(Header, [Header, self = shared_from_this()](){
          auto Header_ = reinterpret_cast<PacketHeader*>(Header->data());
          if(Header_->type == MessageType::IMAGE){
              auto IMG = std::make_shared<std::vector<uint8_t>>(Header_->length);
              self->Session_->Read(IMG, [self, IMG](){
                  std::string jsonStr(IMG->begin(),
                                      IMG->end());
                  auto parsed = nlohmann::json::parse(jsonStr);
                  int jpegSize = parsed.value("jpeg_size", 0);
                  std::string ip = parsed.value("ip", "");
                  auto Body_ = std::make_shared<std::vector<uint8_t>>(jpegSize);
                  self->Session_->Read(Body_, [self, Body_](){
                      self->NetworkManager_.Trigger(MessageType::IMAGE, Body_);
                      self->Read();
                  });
              });
          }else{
              auto Body_ = std::make_shared<std::vector<uint8_t>>(Header_->length);
              self->Session_->Read(Body_, [self, Body_, Header_](){
                  self->NetworkManager_.Trigger(Header_->type, Body_);
                  self->Read();
              });
          }
      });
  }
  void stop() { io_.stop(); }
private:
  boost::asio::io_context &io_;
  std::shared_ptr<Session<Protocol::TCP>> Session_;
  NetworkManager &NetworkManager_;
  ThreadPool *pool_;
};


template <> class Session<Protocol::UDP> {};

#endif // SESSIONENGINE_HPP
