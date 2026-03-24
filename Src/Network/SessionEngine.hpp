#ifndef SESSIONENGINE_HPP
#define SESSIONENGINE_HPP

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "../Thread/ThreadPool.hpp"
#include "Protocol.hpp"
#include "../Crypt/TLS.hpp"
#include "../Crypt/DTLS.hpp"
#include "../Config.hpp"

#include <sigslot.h>
enum class Protocol{
    TCP,
    UDP
};

template<Protocol P>
class Session;


template<>
class Session<Protocol::TCP> : public std::enable_shared_from_this<Session<Protocol::TCP>>{
public:
    explicit Session(boost::asio::io_context &io) : io_(io), strand_(boost::asio::make_strand(io)), socket_(io){};
    void Connect(const std::string ip, const uint16_t port){
        boost::asio::ip::tcp::endpoint endpoint_(boost::asio::ip::make_address(ip), port);
        socket_.async_connect(endpoint_, boost::asio::bind_executor(strand_, [this](const boost::system::error_code &ec){
                                  ///if(ec) {on_error_(ec); return;}
                                  ///
                              }));
    }
    void Read(std::shared_ptr<std::vector<uint8_t>> data, std::function<void()> After){
        boost::asio::async_read(socket_,
                                boost::asio::buffer(data->data(), data->size()),
                                boost::asio::bind_executor(strand_,
                                                           [self = shared_from_this(),
                                                            data = std::move(data),
                                                            After = std::move(After)](const boost::system::error_code& ec, std::size_t bytes){
                                    After();}));
    }
    void Read(std::shared_ptr<std::vector<uint8_t>> data){
        boost::asio::async_read(socket_,
                                boost::asio::buffer(data->data(),data->size()),
                                boost::asio::bind_executor(strand_,
                                                           [self = shared_from_this(),
                                                            data = std::move(data)](const boost::system::error_code &ec, std::size_t bytes){}));
    }

    void Write(std::vector<uint8_t> data){
        auto buf = std::make_shared<std::vector<uint8_t>>(std::move(data));
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(*buf),
                                 boost::asio::bind_executor(strand_,
                                                            [self = shared_from_this(), buf](const boost::system::error_code &ec, std::size_t bytes){}));
    }

private:
    boost::asio::io_context &io_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
};

class ServerConnect : public std::enable_shared_from_this<ServerConnect>{ /// decrypt, encrypt 그리고 Packet 처리
public:
    explicit ServerConnect(const std::string ip, const uint16_t port, ThreadPool& pool) :
        Session_(std::make_shared<Session<Protocol::TCP>>(io_)),
        pool_(pool){
        Session_->Connect(ip, port);
        io_.run();
    }
    void Read(){ /// 읽고 shared_ptr 로 다른 곳에 생명주기가 끊기지 않게 보내주는 역할
        auto header = std::make_shared<std::vector<uint8_t>>(sizeof(anomap::network::PacketHeader));
        Session_->Read(header, [header, self = shared_from_this()](){
            auto Header = reinterpret_cast<anomap::network::PacketHeader*>(header->data());
            auto Body = std::make_shared<std::vector<uint8_t>>(Header->length);
            auto& instance = Config::AppState::getInstance();
            self->Session_->Read(Body);
            switch(Header->type){
                case anomap::network::MessageType::SUCCESS:
                    if(instance.User_ == Config::AppState::User::LogOut){
                        self->pool_.Submit([self, Body_ = Body](){
                            self->Success(Body_);
                        });
                    }else{}
                    break;
                case anomap::network::MessageType::FAIL:
                    if(instance.User_ == Config::AppState::User::LogOut){
                        self->pool_.Submit([self, Body_ = Body](){
                            self->Fail();
                        });
                    }else{}
                    break;
                case anomap::network::MessageType::AVAILABLE:
                    if(instance.User_ != Config::AppState::User::LogOut){
                        self->pool_.Submit([self](){
                            self->Available();
                        });
                    }else{

                    }
                    break;
                case anomap::network::MessageType::CAMERA:
                    if(instance.User_ != Config::AppState::User::LogOut){
                        self->pool_.Submit([self](){
                            self->Camera();
                        });
                    }else{

                    }
                    break;
                case anomap::network::MessageType::AI:
                    if(instance.User_ != Config::AppState::User::LogOut){
                        self->pool_.Submit([self](){
                            self->AI();
                        });
                    }else{

                    }
                    break;
                case anomap::network::MessageType::META:
                    if(instance.User_ != Config::AppState::User::LogOut){
                        self->pool_.Submit([self](){
                            self->Available();
                        });
                    }else{

                    }
                    break;
                case anomap::network::MessageType::IMAGE:
                    if(instance.User_ != Config::AppState::User::LogOut){
                        self->pool_.Submit([self](){
                            self->Image();
                        });
                    }else{

                    }
                    break;
        };
        self->Read();
        });
    };
    void Write();
    void Login();
    void Success(std::shared_ptr<std::vector<uint8_t>> Body_){
        std::string json_str(Body_->begin(), Body_->end());
        nlohmann::json json_ = nlohmann::json::parse(json_str);
        auto& instance = Config::AppState::getInstance();
        if(json_["state"] == "admin"){
            instance.User_ = Config::AppState::User::Admin;
        }else{
            instance.User_ = Config::AppState::User::Normal;
        }
        /// SigSlot 쓰기
    }
    void Fail();
    void Camera();
    void Meta();
    void Image();
    void Available();
    void Device();
    void AI();
private:
    boost::asio::io_context io_;
    std::shared_ptr<Session<Protocol::TCP>> Session_;
    ThreadPool& pool_;
    std::map<anomap::network::MessageType, std::tuple<>> map_;

};

class NetworkManager{ /// App 의 State 관리, Read 와 Write 를 분리하여 Login -> Write 로 연결 Read 시에는?
    /// 직관적으로 SUCCESS || FAIL 은 Login 후에 돌아오는게 직관적이다. 이걸 직접 코드로써 보이게 할려면? wait_Read 같은게 필요할 듯 하다. 하지만
    /// 어떤 값이 와야하나? Network Packet 인가? True || False 값인가? 아마 True || False 값일 것이다.
    /// 실제로는 완전 비동기적으로 동작을 하나 Thread 도 잡아먹진 않으나 하나의 코드 블럭에서 Write 와 Read 가 이뤄져야한다.
    /// NetworkManager 는 외부의 클래스들에게 어떻게 의존성이 부여가 되어야하나? 과도한 Callback 함수가 이뤄지진 않을까? QObject 에서 오는 콜백 함수와 Pure C++ 클래스의 콜백 함수는 서로 섞여도 괜찮은가?
    /// 전체 데이터에 대한 이해도가 없는 상태인가?
    /// CameraID{ crop, ip , AIStruct, DeviceStruct},
    /// AIStruct{이미지들이 있어야함}
    /// DeviceStruct{조도,모터,CPU, Mem, Temp}
    /// Server{CPU, Mem, Temp}
    /// 얘네들은 QtBack 이 어울리나? PureC++ 가 어울리나? 주기적으로 갱신이 되고 QObject 가 값을 렌더링마다 가져가는 형태여야한다.
    /// ㅇㅋ 모든 구조체는 C++ 코드로 작성하고 QObject Bridge 클래스들은 전부 UI 에서 필요한 정보를 가져오기 위한 signal slot 으로만 남겨둔다.
    /// QObject 는 PureC++ 를 바라보는 단방향성 구조로 전환한다. 서로 상호소통하는 것이 아니다(필요시에는 하는게 맞음)

};

template<>
class Session<Protocol::UDP>{

};

#endif // SESSIONENGINE_HPP
