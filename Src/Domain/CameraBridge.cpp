#include "CameraBridge.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

CameraBridge::CameraBridge(CameraManager& cameraManager,
                           NetworkManager& networkManager)
    : CameraManager_(cameraManager), NetworkManager_(networkManager)
{
    NetworkManager_.CameraBridge_.connect([this](std::string ip, CameraPayload payload){
        std::visit(overloaded{
            [&](const std::string& rtsp){
                auto camera = CameraManager_.GetByIp(ip);
                if(!camera) CameraManager_.Add(ip, rtsp);
                else camera->rtsp_ = rtsp;
            },
            [&](const std::vector<uint8_t>& img){
                for(auto id : CameraManager_.GetIdsByIp(ip)){
                    auto camera = CameraManager_.Get(id);
                    if(camera) camera->Images_.Add(img);
                }
            },
            [&](const std::pair<std::array<float, 4>, std::string>& meta_pair){
                for(auto id : CameraManager_.GetIdsByIp(ip)){
                    auto camera = CameraManager_.Get(id);
                    if(camera) camera->MetaData_.Add(meta_pair.first, meta_pair.second);
                }
            },
            [&](const ImageMeta& meta){
                for(auto id : CameraManager_.GetIdsByIp(ip)){
                    auto camera = CameraManager_.Get(id);
                    if(camera) camera->Images_.SetMeta(meta.jpeg_size, meta.total_frames);
                }
            },
        }, payload);
    });

    NetworkManager_.UdpImageBridge_.connect([this](const std::string& ip, const ImageHeader& header, const uint8_t* imageData, size_t imageSize){
        for(auto id : CameraManager_.GetIdsByIp(ip)){
            auto camera = CameraManager_.Get(id);
            if(!camera) continue;

            camera->Images_.Feed(header, imageData, imageSize);

            if(camera->Images_.isFrameComplete(header.FrameNumber)){
                camera->Images_.ExtractFrame(header.FrameNumber);
                NetworkManager_.OutboundImageBridge_(R"({"receive":true})");
            } else if (header.SequenceNumber == header.MaxSequenceNumber) {
                std::string nack = camera->Images_.BuildResultJson(header.FrameNumber);
                NetworkManager_.OutboundImageBridge_(nack);
            }
        }
    });
}
