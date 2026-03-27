#pragma once
#include "IStore.hpp"
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <set>
#include "../Network/SessionEngine.hpp"

struct CameraMeta{
    std::deque<float> tmp_{20, 0.0f}, tilt_{20, 0.0f}, light_{20, 0.0f}, hum_{20, 0.0f};
    std::string dir{""};
    void Add(std::initializer_list<float> meta, std::string str){
        dir = str;
        if (meta.size() < 4) return;
        auto it = meta.begin();
        tmp_.pop_front();
        tmp_.push_back(*it++);
        tilt_.pop_front();
        tilt_.push_back(*it++);
        light_.pop_front();
        light_.push_back(*it++);
        hum_.pop_front();
        hum_.push_back(*it++);
    }
};

struct CameraImage{
    using IMG = std::vector<uint8_t>;
    std::deque<IMG> ImageList_;
    void Add(IMG img){
        if(ImageList_.size() == 20){
            ImageList_.pop_front();
        }
        ImageList_.push_back(img);
    }
};

struct CameraInfo{
    std::string rtsp_{""}, ip_{""};
    CameraMeta MetaData_;
    CameraImage Images_;
};


class CameraManager{
public:
    CameraManager();
    ~CameraManager();

    void Add(std::string ip, std::string rtsp){
        auto& indices = ipTable_[ip];
        uint8_t nextIndex = 0;
        while (indices.count(nextIndex)) { /// set을 순회하며 비어있는(가장 작은) 번호 찾기
            nextIndex++;
        }
        indices.insert(nextIndex);
        std::string id = ip + "/" + std::to_string(nextIndex);/// 2. ID 생성 (예: 192.168.0.1/0)
        map_[id] = {
            .rtsp_ = rtsp,
            .ip_ = ip
        };
    }

    void Remove(std::string id){
        auto it = map_.find(id);
        if (it == map_.end()) return;
        size_t slashPos = id.find_last_of('/'); /// 1. ID에서 IP와 인덱스 분리 (예: "192.168.0.1/0" -> "192.168.0.1", 0)
        if (slashPos != std::string::npos) {
            std::string ip = id.substr(0, slashPos);
            int index = std::stoi(id.substr(slashPos + 1));
            ipTable_[ip].erase(index); /// 2. 사용 중인 인덱스 집합에서 해당 번호 제거 (나중에 다시 쓰일 수 있게)
            if (ipTable_[ip].empty()) { /// 만약 해당 IP의 모든 카메라가 삭제되었다면 IP 테이블에서도 정리
                ipTable_.erase(ip);
            }
        }
        map_.erase(it); /// 3. 실제 카메라 데이터 삭제
    }
    std::map<std::string, CameraInfo>& getCameras(){
        return map_;
    }
private:
    std::map<std::string, CameraInfo> map_; /// id by Camera
    std::map<std::string, std::set<uint8_t>> ipTable_; /// ip by index
};

class CameraBridge{

    // using CameraPayload = std::variant<std::string,
    //                                    std::vector<uint8_t>,
    //                                    std::pair<std::vector<float>,std::string>>;
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
public:
    CameraBridge(CameraManager& cameraManager_, NetworkManager& networkManager) : CameraManager_(cameraManager_), NetworkManager_(networkManager){
        NetworkManager_.CameraBridge_.connect([this](std::string id, CameraPayload payload){
            std::visit(overloaded{
                [&](const std::string& str){},
                [&](const std::vector<uint8_t>&){},
                [&](const std::pair<std::vector<float>, std::string>&){},
                }, payload);
        });
    };
private:
    CameraManager& CameraManager_;
    NetworkManager& NetworkManager_;
};
