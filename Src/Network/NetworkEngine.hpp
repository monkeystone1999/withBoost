#pragma once
#include "NetworkProtocol.hpp"
#include "../Thread/ThreadEngine.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <functional>
#include <variant>
#include <deque>
#include <string>

/**
 * @file NetworkEngine.hpp
 * @brief 저수준 네트워크 통신 엔진 (Session, Connect 계열)
 */

class NetworkManager;

// ─────────────────────────────────────────────────────────────
// Session — TCP/UDP 저수준 통신 세션
// ─────────────────────────────────────────────────────────────

template <Protocol P> class Session;

template <>
class Session<Protocol::TCP> : public std::enable_shared_from_this<Session<Protocol::TCP>> {
public:
    explicit Session(boost::asio::io_context &io)
        : io_(io), strand_(boost::asio::make_strand(io)), socket_(io) {};

    void Connect(const std::string ip, const uint16_t port) {
        boost::asio::ip::tcp::endpoint endpoint_(boost::asio::ip::make_address(ip), port);
        socket_.async_connect(endpoint_, boost::asio::bind_executor(strand_, [](const boost::system::error_code &ec) {}));
    }

    void Read(std::shared_ptr<std::vector<uint8_t>> data, std::function<void()> After) {
        boost::asio::async_read(socket_, boost::asio::buffer(data->data(), data->size()),
            boost::asio::bind_executor(strand_, [self = shared_from_this(), data, After](const boost::system::error_code &ec, std::size_t bytes) {
                if (!ec) After();
            }));
    }

    void Read(std::shared_ptr<std::vector<uint8_t>> data) {
        boost::asio::async_read(socket_, boost::asio::buffer(data->data(), data->size()),
            boost::asio::bind_executor(strand_, [](const boost::system::error_code &ec, std::size_t bytes) {}));
    }

    void Write(std::vector<uint8_t> data) {
        auto buf = std::make_shared<std::vector<uint8_t>>(std::move(data));
        boost::asio::async_write(socket_, boost::asio::buffer(*buf),
            boost::asio::bind_executor(strand_, [self = shared_from_this(), buf](const boost::system::error_code &ec, std::size_t bytes) {}));
    }

    void stop() {
        boost::system::error_code ec;
        socket_.close(ec);
    }

private:
    boost::asio::io_context &io_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
};

template <>
class Session<Protocol::UDP> : public std::enable_shared_from_this<Session<Protocol::UDP>> {
public:
    explicit Session(boost::asio::io_context &io)
        : io_(io), strand_(boost::asio::make_strand(io)), socket_(io, boost::asio::ip::udp::v4()) {}

    void Bind(uint16_t port) {
        boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(), port);
        socket_.bind(endpoint);
    }

    void Bind() {
        boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(), 0);
        socket_.bind(endpoint);
    }

    uint16_t LocalPort() const { return socket_.local_endpoint().port(); }

    void Connect(const std::string& ip, uint16_t port) {
        endpoint_ = boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(ip), port);
    }

    void Write(std::vector<uint8_t> data) {
        auto buf = std::make_shared<std::vector<uint8_t>>(std::move(data));
        socket_.async_send_to(boost::asio::buffer(*buf), endpoint_,
            boost::asio::bind_executor(strand_, [self = shared_from_this(), buf](const boost::system::error_code& ec, std::size_t bytes) {}));
    }

    using ReceiveCallback = std::function<void(std::shared_ptr<std::vector<uint8_t>>, std::size_t, boost::asio::ip::udp::endpoint)>;

    void StartReceive(ReceiveCallback cb) {
        receiveCb_ = std::move(cb);
        doReceive();
    }

    void stop() {
        boost::system::error_code ec;
        socket_.close(ec);
    }

private:
    void doReceive() {
        auto buf = std::make_shared<std::vector<uint8_t>>(2048);
        auto sender = std::make_shared<boost::asio::ip::udp::endpoint>();
        socket_.async_receive_from(boost::asio::buffer(*buf), *sender,
            boost::asio::bind_executor(strand_, [self = shared_from_this(), buf, sender](const boost::system::error_code& ec, std::size_t bytes) {
                if (!ec && bytes > 0 && self->receiveCb_) {
                    self->receiveCb_(buf, bytes, *sender);
                }
                if (!ec) self->doReceive();
            }));
    }

    boost::asio::io_context &io_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint endpoint_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    ReceiveCallback receiveCb_;
};

// ─────────────────────────────────────────────────────────────
// Connection Handlers — 고수준 세션 엔진
// ─────────────────────────────────────────────────────────────

class MediaConnect;

class TextConnect : public std::enable_shared_from_this<TextConnect> {
public:
    TextConnect(NetworkManager& Manager, std::string ip, uint16_t port);
    void run(ThreadEngine* pool = nullptr);
    void setMediaConnect(std::shared_ptr<MediaConnect> media);
    void Read();
    void Send(MessageType type, const std::string& jsonBody);
    void SendImageReceiveResult(const std::string& jsonBody);
    const std::string& getCurrentImageIp() const { return currentImageIp_; }
    void stop();

private:
    uint16_t getMediaPort() const;
    boost::asio::io_context &io_;
    std::shared_ptr<Session<Protocol::TCP>> TCPSession_;
    NetworkManager &NetworkManager_;
    ThreadEngine *pool_ = nullptr;
    std::shared_ptr<MediaConnect> MediaConnect_;
    std::string currentImageIp_;
};

class MediaConnect : public std::enable_shared_from_this<MediaConnect> {
public:
    explicit MediaConnect(NetworkManager& Manager);
    uint16_t LocalPort() const;
    void run(ThreadEngine* pool = nullptr);
    void Read();
    void Write(std::vector<uint8_t> data);
    void stop();

private:
    void onReceive(std::shared_ptr<std::vector<uint8_t>> buf, std::size_t bytes, boost::asio::ip::udp::endpoint sender);

    boost::asio::io_context &io_;
    std::shared_ptr<Session<Protocol::UDP>> UDPSession_;
    NetworkManager &NetworkManager_;
    ThreadEngine *pool_ = nullptr;
};

using ServerConnect = TextConnect;
