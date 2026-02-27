#pragma once
#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;
enum class Protocol{
    TCP = 0x01,
    UDP = 0x02
};
struct V5ctx{
    //SSL_CTX* ssl_ctx;
    boost::asio::io_context io;
    tcp::resolver endpoint;
};

class v5Context{
public:
    v5Context() : io() ,resolver(io){}
    auto Endpoint(const std::string ip, const std::string port){
        auto result = resolver.resolve(ip.data(), port.data());
        return result;
    }
    boost::asio::io_context io;
    tcp::resolver resolver;
};

class Session{
public:
    explicit Session(boost::asio::io_context& io, const tcp::resolver::results_type& endpoint) : io_(io){

    }
    ~Session(){
        socket_.close();
    }
private:
    boost::asio::io_context& io_;
    tcp::socket socket_;
};

class ConnectServer{
public:
    ConnectServer(v5Context& ctx, const std::string ip, const std::string port) : ConnectServer(ctx, Prepare(ctx, ip, port) ) {}
    int read(std::string&);
    int write(const std::string_view);
private:
    ConnectServer(v5Context& ctx, auto endpoint) : session_(ctx.io, endpoint){

    }
    static auto Prepare(v5Context& ctx, const std::string ip, const std::string port){
        return ctx.Endpoint(ip,port);
    }
private:
    std::string ip, port;
    Session session_;
};

