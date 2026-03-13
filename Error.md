# Network Packet UI 통합 요구사항 명세서

**작성일**: 2026-03-12  
**프로젝트**: AnoMAP - Camera Monitoring System  
**목표**: Server→Qt 네트워크 패킷의 UI 통합 및 IMAGE 패킷 수신 대비

---

## 📋 목차

1. [현재 상태 분석](#현재-상태-분석)
2. [요구사항 개요](#요구사항-개요)
3. [상세 구현 명세](#상세-구현-명세)
4. [파일별 수정 사항](#파일별-수정-사항)
5. [테스트 시나리오](#테스트-시나리오)

---

## 🔍 현재 상태 분석

### **Network.json 패킷 정의**

| 패킷 타입 | 방향 | 구현 상태 | UI 통합 |
|----------|------|----------|---------|
| LOGIN | Qt→Server | ✅ 완료 | ✅ LoginPage |
| SUCCESS | Server→Qt | ✅ 완료 | ✅ 로그인 성공 처리 |
| FAIL | Server→Qt | ✅ 완료 | ✅ 에러 메시지 표시 |
| DEVICE | Qt→Server | ✅ 완료 | ✅ DevicePage Controls |
| **AVAILABLE** | **Server→Qt** | ✅ 완료 | ⚠️ **부분 구현** |
| AI | Server→Qt | ✅ 완료 | ✅ AIPage |
| CAMERA | Server→Qt | ✅ 완료 | ✅ DashboardPage |
| ASSIGN | 양방향 | ✅ 완료 | ✅ AdminPage (User) |
| META | Server→Qt | ⚠️ 정의만 | ❌ 미구현 |
| **IMAGE** | **Server→Qt** | ❌ **미구현** | ❌ **미구현** |

### **AVAILABLE 패킷 현재 구현 상태**

**Network.json 정의**:
```json
{
  "server": {
    "cpu": 0.0,
    "memory": 0.0,
    "temp": 0.0,
    "uptime": 0
  },
  "devices": [
    {
      "ip": "192.168.0.43",
      "cpu": 92.365645,
      "memory": 29.522263,
      "temp": 51.121,
      "uptime": 90730,
      "pending_events": 0
    }
  ]
}
```

**현재 구현**:
- ✅ `DeviceStore`: IP 기반 통합 관리, History 최대 20개 저장
- ✅ `ServerStatusStore`: Server + Devices 정보 저장
- ✅ `DeviceModel`: QML에서 Device 정보 조회
- ✅ `ServerStatusModel`: QML에서 Server 정보 조회
- ⚠️ **DevicePage**: 선택한 1개 Device만 표시
- ❌ **AdminPage**: Server 정보 미표시

### **IMAGE 패킷 현재 상태**

**Network.json 정의**:
```json
{
  "device_id": "string",
  "track_id": 0,
  "frame_index": 0,
  "total_frames": 0,
  "timestamp_ms": 0,
  "jpeg_size": 0
}
```

**현재 구현**:
- ❌ `Header.hpp`에 `IMAGE` MessageType 없음
- ❌ `NetworkService`에 IMAGE 패킷 처리 없음
- ❌ UI 표시 없음

---

## 📌 요구사항 개요

### **요구사항 1: DevicePage에 Device 목록 표시**

**목표**: AVAILABLE."devices" 배열의 모든 Device를 10개 버퍼로 표시

**현재**: 선택한 1개 Camera의 Device 정보만 표시  
**변경 후**: 모든 Device를 리스트/그리드로 표시, 최대 10개 버퍼

**UI 위치**: DevicePage 왼쪽 패널 또는 상단 패널

### **요구사항 2: AdminPage에 Server 정보 표시**

**목표**: Admin 로그인 시 AVAILABLE."server" 정보를 AdminPage에 표시

**현재**: AdminPage는 User Management만 표시  
**변경 후**: Server CPU/Memory/Temp/Uptime 카드 추가

**UI 위치**: AdminPage 상단 또는 별도 섹션

### **요구사항 3: IMAGE 패킷 수신 대비**

**목표**: IMAGE 패킷 raw data 수신 인프라 구축

**현재**: 미구현  
**변경 후**: 
- Header.hpp에 IMAGE MessageType 추가
- NetworkService에 IMAGE 패킷 파싱 추가
- ImageStore 생성 (버퍼 관리)
- QML에서 표시할 수 있는 구조 준비

---

## 📐 상세 구현 명세

### **요구사항 1: DevicePage Device 목록 표시**

#### **1.1 UI 레이아웃 변경**

**파일**: `Qt/Front/View/DevicePage.qml`

**변경 사항**:
```qml
// 현재: 2열 레이아웃 (Camera Grid 70% | Device Detail 30%)
// 변경: 3열 레이아웃 (Device List 20% | Camera Grid 50% | Device Detail 30%)

RowLayout {
    // ✅ 새로 추가: 왼쪽 Device List 패널
    Rectangle {
        Layout.fillHeight: true
        Layout.preferredWidth: parent.width * 0.2
        color: Theme.bgSecondary
        
        // Device List (최대 10개)
        DeviceListPanel {
            anchors.fill: parent
            model: deviceModel
            maxDevices: 10
            selectedIp: root.selectedIp
            onDeviceSelected: ip => {
                root.selectedIp = ip;
                // 해당 IP의 Camera 자동 선택
            }
        }
    }
    
    // Camera Grid (기존, 폭 축소)
    Rectangle {
        Layout.fillHeight: true
        Layout.preferredWidth: parent.width * 0.5  // 70% → 50%
        // ...
    }
    
    // Device Detail (기존 유지)
    Rectangle {
        Layout.fillHeight: true
        Layout.preferredWidth: parent.width * 0.3
        // ...
    }
}
```

#### **1.2 DeviceListPanel 컴포넌트 생성**

**파일**: `Qt/Front/Component/device/DeviceListPanel.qml` (신규)

**구현 내용**:
```qml
import QtQuick
import QtQuick.Controls
import AnoMap.front

Item {
    id: root
    property var model  // DeviceModel
    property int maxDevices: 10
    property string selectedIp: ""
    
    signal deviceSelected(string ip)
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: Theme.bgPrimary
            Text {
                anchors.centerIn: parent
                text: "Devices"
                font.pixelSize: 14
                font.bold: true
                color: Theme.fontColor
            }
        }
        
        // Device List (최대 10개)
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.model
            clip: true
            
            // 최대 10개로 제한
            delegate: Item {
                width: ListView.view.width
                height: 60
                visible: index < root.maxDevices
                
                DeviceListItem {
                    anchors.fill: parent
                    ip: model.ip
                    isOnline: model.isOnline
                    cpu: model.cpu
                    memory: model.memory
                    temp: model.temp
                    selected: root.selectedIp === model.ip
                    
                    onClicked: root.deviceSelected(model.ip)
                }
            }
        }
        
        // "More devices..." 표시 (10개 초과 시)
        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: Theme.bgTertiary
            visible: root.model && root.model.rowCount() > root.maxDevices
            
            Text {
                anchors.centerIn: parent
                text: "+ " + (root.model.rowCount() - root.maxDevices) + " more devices"
                color: Theme.isDark ? "#888" : "#666"
                font.pixelSize: 11
            }
        }
    }
}
```

#### **1.3 DeviceListItem 컴포넌트 생성**

**파일**: `Qt/Front/Component/device/DeviceListItem.qml` (신규)

**구현 내용**:
```qml
import QtQuick
import QtQuick.Controls
import AnoMap.front

Rectangle {
    id: root
    property string ip: ""
    property bool isOnline: false
    property double cpu: 0
    property double memory: 0
    property double temp: 0
    property bool selected: false
    
    signal clicked()
    
    color: selected ? Theme.hanwhaFirst : (isOnline ? Theme.bgPrimary : Theme.bgSecondary)
    border.width: selected ? 2 : 0
    border.color: Theme.hanwhaSecond
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4
        
        // IP + Status
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: root.ip
                color: selected ? "white" : Theme.fontColor
                font.pixelSize: 12
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: isOnline ? "#4caf50" : "#f44336"
            }
        }
        
        // CPU / Memory / Temp (Compact)
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Text {
                text: "CPU:" + cpu.toFixed(0) + "%"
                color: selected ? "#ddd" : "#888"
                font.pixelSize: 9
            }
            Text {
                text: "M:" + memory.toFixed(0) + "%"
                color: selected ? "#ddd" : "#888"
                font.pixelSize: 9
            }
            Text {
                text: "T:" + temp.toFixed(0) + "°C"
                color: selected ? "#ddd" : "#888"
                font.pixelSize: 9
            }
        }
    }
    
    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
```

---

### **요구사항 2: AdminPage Server 정보 표시**

#### **2.1 AdminPage 레이아웃 변경**

**파일**: `Qt/Front/View/AdminPage.qml`

**변경 사항**:
```qml
Item {
    id: root
    
    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            
            // ✅ 새로 추가: Server Status 섹션
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: Theme.bgSecondary
                visible: typeof serverStatusModel !== "undefined" && serverStatusModel.hasServer
                
                ServerStatusPanel {
                    anchors.fill: parent
                    serverCpu: serverStatusModel.serverCpu
                    serverMemory: serverStatusModel.serverMemory
                    serverTemp: serverStatusModel.serverTemp
                }
            }
            
            // Header (기존)
            Rectangle {
                id: header
                Layout.fillWidth: true
                height: 56
                // ...
            }
            
            // User List (기존)
            UserListLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                // ...
            }
        }
    }
}
```

#### **2.2 ServerStatusPanel 컴포넌트 생성**

**파일**: `Qt/Front/Component/admin/ServerStatusPanel.qml` (신규)

**구현 내용**:
```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Rectangle {
    id: root
    property double serverCpu: 0
    property double serverMemory: 0
    property double serverTemp: 0
    
    color: Theme.bgSecondary
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12
        
        // Header
        Text {
            text: "🖥️  Server Status"
            font.pixelSize: 18
            font.bold: true
            color: Theme.fontColor
        }
        
        // Status Cards
        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            
            StatusCard {
                title: "CPU Usage"
                value: root.serverCpu.toFixed(1) + "%"
                color: root.serverCpu > 80 ? "#f44336" : "#4caf50"
            }
            
            StatusCard {
                title: "Memory Usage"
                value: root.serverMemory.toFixed(1) + "%"
                color: root.serverMemory > 80 ? "#f44336" : "#4caf50"
            }
            
            StatusCard {
                title: "Temperature"
                value: root.serverTemp.toFixed(1) + "°C"
                color: root.serverTemp > 70 ? "#f44336" : "#4caf50"
            }
        }
    }
    
    component StatusCard: Rectangle {
        property string title: ""
        property string value: ""
        property color color: "#4caf50"
        
        Layout.fillWidth: true
        height: 80
        color: Theme.bgPrimary
        radius: 8
        border.width: 2
        border.color: parent.color
        
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 4
            
            Text {
                text: title
                color: "#888"
                font.pixelSize: 11
                Layout.alignment: Qt.AlignHCenter
            }
            
            Text {
                text: value
                color: Theme.fontColor
                font.pixelSize: 22
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
```

---

### **요구사항 3: IMAGE 패킷 수신 대비**

#### **3.1 Header.hpp에 IMAGE MessageType 추가**

**파일**: `Src/Network/Header.hpp`

**변경 전**:
```cpp
enum class MessageType : uint8_t {
  ACK = 0x00,
  LOGIN = 0x01,
  SUCCESS = 0x02,
  FAIL = 0x03,
  DEVICE = 0x04,
  AVAILABLE = 0x05,
  AI = 0x06,
  CAMERA = 0x07,
  ASSIGN = 0x08
};
```

**변경 후**:
```cpp
enum class MessageType : uint8_t {
  ACK = 0x00,
  LOGIN = 0x01,
  SUCCESS = 0x02,
  FAIL = 0x03,
  DEVICE = 0x04,
  AVAILABLE = 0x05,
  AI = 0x06,
  CAMERA = 0x07,
  ASSIGN = 0x08,
  META = 0x09,        // ✅ 추가
  IMAGE = 0x0a        // ✅ 추가
};
```

#### **3.2 ImageStore 생성**

**파일**: `Src/Domain/Image.hpp` (신규)

**구현 내용**:
```cpp
#pragma once
#include "IStore.hpp"
#include <mutex>
#include <string>
#include <vector>

// --- Image Frame Data ---
struct ImageFrame {
  std::string deviceId;
  int trackId = 0;
  int frameIndex = 0;
  int totalFrames = 0;
  int64_t timestampMs = 0;
  
  // Raw JPEG data
  std::vector<uint8_t> jpegData;
  int jpegSize = 0;
};

// --- ImageStore (10-frame circular buffer per device) ---
class ImageStore : public IStore<std::vector<ImageFrame>> {
public:
  void updateFromJson(const std::string &json, Callback cb) override;
  
  // Binary data 추가 (JSON 헤더 + raw JPEG data)
  void addImageData(const std::string &jsonHeader, 
                    const std::vector<uint8_t> &jpegData);
  
  std::vector<ImageFrame> snapshot() const override;
  
  // Device별 최신 이미지 조회
  ImageFrame getLatestImage(const std::string &deviceId) const;

private:
  mutable std::mutex mutex_;
  std::vector<ImageFrame> images_;  // 최대 10개 버퍼
  static constexpr size_t MAX_IMAGES = 10;
};
```

**파일**: `Src/Domain/Image.cpp` (신규)

**구현 내용**:
```cpp
#include "Image.hpp"
#include <nlohmann/json.hpp>

void ImageStore::updateFromJson(const std::string &json, Callback cb) {
  // JSON 헤더만 파싱 (실제 JPEG data는 별도)
  auto parsed = nlohmann::json::parse(json, nullptr, false);
  if (!parsed.is_object())
    return;
  
  ImageFrame frame;
  frame.deviceId = parsed.value("device_id", "");
  frame.trackId = parsed.value("track_id", 0);
  frame.frameIndex = parsed.value("frame_index", 0);
  frame.totalFrames = parsed.value("total_frames", 0);
  frame.timestampMs = parsed.value("timestamp_ms", 0);
  frame.jpegSize = parsed.value("jpeg_size", 0);
  
  // jpegData는 addImageData에서 추가
  
  if (cb)
    cb(snapshot());
}

void ImageStore::addImageData(const std::string &jsonHeader, 
                              const std::vector<uint8_t> &jpegData) {
  auto parsed = nlohmann::json::parse(jsonHeader, nullptr, false);
  if (!parsed.is_object())
    return;
  
  ImageFrame frame;
  frame.deviceId = parsed.value("device_id", "");
  frame.trackId = parsed.value("track_id", 0);
  frame.frameIndex = parsed.value("frame_index", 0);
  frame.totalFrames = parsed.value("total_frames", 0);
  frame.timestampMs = parsed.value("timestamp_ms", 0);
  frame.jpegSize = parsed.value("jpeg_size", 0);
  frame.jpegData = jpegData;
  
  std::lock_guard<std::mutex> lk(mutex_);
  
  // FIFO 버퍼 (최대 10개)
  images_.push_back(std::move(frame));
  if (images_.size() > MAX_IMAGES) {
    images_.erase(images_.begin());
  }
}

std::vector<ImageFrame> ImageStore::snapshot() const {
  std::lock_guard<std::mutex> lk(mutex_);
  return images_;
}

ImageFrame ImageStore::getLatestImage(const std::string &deviceId) const {
  std::lock_guard<std::mutex> lk(mutex_);
  
  // 역순 탐색 (최신 것 먼저)
  for (auto it = images_.rbegin(); it != images_.rend(); ++it) {
    if (it->deviceId == deviceId) {
      return *it;
    }
  }
  
  return ImageFrame{}; // 없으면 빈 객체
}
```

#### **3.3 NetworkService에 IMAGE 패킷 처리 추가**

**파일**: `Src/Network/NetworkService.cpp`

**변경 위치**: `handleMessage()` 함수의 switch 문

**추가 코드**:
```cpp
case MessageType::IMAGE: {
  // IMAGE 패킷은 JSON 헤더 + raw JPEG data 구조
  // 1. JSON 헤더 파싱
  std::string jsonHeader(body.begin(), body.end());
  
  // 2. JPEG 데이터는 다음 패킷 또는 동일 패킷 내 포함
  // (서버 구현에 따라 다름 - 현재는 분리된 것으로 가정)
  
  // 3. ImageStore에 전달
  if (imageCallback_) {
    imageCallback_(jsonHeader);
  }
  break;
}

case MessageType::META: {
  // META 패킷 처리 (센서 데이터)
  std::string json(body.begin(), body.end());
  if (metaCallback_) {
    metaCallback_(json);
  }
  break;
}
```

#### **3.4 Qt Layer ImageModel 생성**

**파일**: `Qt/Back/Image.hpp` (신규)

**구현 내용**:
```cpp
#pragma once
#include "../../Src/Domain/Image.hpp"
#include <QAbstractListModel>
#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QString>

struct ImageEntry {
  QString deviceId;
  int trackId = 0;
  int frameIndex = 0;
  int totalFrames = 0;
  qint64 timestampMs = 0;
  QByteArray jpegData;
  QImage image;  // QImage로 디코딩된 데이터
};

class ImageModel : public QAbstractListModel {
  Q_OBJECT
public:
  enum Roles {
    DeviceIdRole = Qt::UserRole + 1,
    TrackIdRole,
    FrameIndexRole,
    TotalFramesRole,
    TimestampRole,
    ImageRole
  };
  
  explicit ImageModel(QObject *parent = nullptr);
  
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  
  // QML에서 최신 이미지 조회
  Q_INVOKABLE QImage getLatestImage(const QString &deviceId) const;

public slots:
  void onStoreUpdated(std::vector<ImageFrame> snapshot);

private:
  QList<ImageEntry> images_;
};
```

---

## 📝 파일별 수정 사항

### **신규 생성 파일 (9개)**

| 파일 경로 | 설명 |
|----------|------|
| `Qt/Front/Component/device/DeviceListPanel.qml` | Device 목록 패널 (최대 10개) |
| `Qt/Front/Component/device/DeviceListItem.qml` | Device 리스트 아이템 |
| `Qt/Front/Component/admin/ServerStatusPanel.qml` | Server 상태 표시 패널 |
| `Src/Domain/Image.hpp` | IMAGE 패킷 Domain 레이어 |
| `Src/Domain/Image.cpp` | IMAGE 패킷 처리 로직 |
| `Qt/Back/Image.hpp` | IMAGE 패킷 Qt 레이어 |
| `Qt/Back/Image.cpp` | ImageModel 구현 |

### **수정 파일 (5개)**

| 파일 경로 | 수정 내용 |
|----------|----------|
| `Src/Network/Header.hpp` | MessageType에 META, IMAGE 추가 |
| `Src/Network/NetworkService.cpp` | IMAGE, META 패킷 처리 추가 |
| `Qt/Front/View/DevicePage.qml` | 3열 레이아웃으로 변경, DeviceListPanel 추가 |
| `Qt/Front/View/AdminPage.qml` | ServerStatusPanel 추가 |
| `Core.cpp` | ImageStore 초기화 및 연결 |

---

## 🧪 테스트 시나리오

### **시나리오 1: Device 목록 표시**

**준비**:
1. Server에서 AVAILABLE 패킷 전송 (devices 배열 5개)

**기대 결과**:
1. DevicePage 왼쪽 패널에 5개 Device 표시
2. 각 Device는 IP, CPU, Memory, Temp 표시
3. Device 클릭 시 오른쪽 상세 패널에 정보 표시
4. 10개 초과 시 "More devices..." 표시

### **시나리오 2: Admin Server 정보 표시**

**준비**:
1. Admin 계정으로 로그인
2. Server에서 AVAILABLE 패킷 전송 (server 객체 포함)

**기대 결과**:
1. AdminPage 상단에 ServerStatusPanel 표시
2. Server CPU/Memory/Temp 실시간 업데이트
3. 80% 초과 시 빨간색 경고

### **시나리오 3: IMAGE 패킷 수신**

**준비**:
1. Server에서 IMAGE 패킷 전송 (JSON 헤더 + JPEG data)

**기대 결과**:
1. ImageStore에 데이터 저장 (최대 10개)
2. ImageModel에서 QImage로 디코딩
3. QML에서 Image 컴포넌트로 표시 가능

---

## 📊 구현 우선순위

### **Phase 1: Device 목록 표시** (High Priority)
1. DeviceListPanel 컴포넌트 생성
2. DeviceListItem 컴포넌트 생성
3. DevicePage 레이아웃 변경
4. 테스트 및 검증

### **Phase 2: Admin Server 정보** (Medium Priority)
1. ServerStatusPanel 컴포넌트 생성
2. AdminPage 레이아웃 변경
3. 테스트 및 검증

### **Phase 3: IMAGE 패킷 대비** (Low Priority)
1. Header.hpp 수정
2. ImageStore 생성
3. NetworkService 수정
4. ImageModel 생성
5. 통합 테스트

---

## ✅ 완료 체크리스트

### **Phase 1: Device 목록**
- [ ] DeviceListPanel.qml 생성
- [ ] DeviceListItem.qml 생성
- [ ] DevicePage.qml 레이아웃 변경
- [ ] 테스트: 10개 Device 표시
- [ ] 테스트: Device 선택 시 상세 표시

### **Phase 2: Admin Server**
- [ ] ServerStatusPanel.qml 생성
- [ ] AdminPage.qml 레이아웃 변경
- [ ] 테스트: Admin 로그인 시 Server 정보 표시
- [ ] 테스트: 실시간 업데이트

### **Phase 3: IMAGE 패킷**
- [ ] Header.hpp 수정
- [ ] Image.hpp/cpp 생성
- [ ] Image.hpp/cpp (Qt) 생성
- [ ] NetworkService.cpp 수정
- [ ] Core.cpp 연결
- [ ] 테스트: IMAGE 패킷 수신
- [ ] 테스트: QML 표시

---

## 📝 주의사항

### **1. Device 목록 버퍼 크기**
- 최대 10개로 제한
- 10개 초과 시 "More devices..." 표시
- 향후 페이징 또는 스크롤 기능 추가 고려

### **2. Server 정보 Admin 전용**
- `serverStatusModel.hasServer` 조건으로 표시
- User 계정에서는 미표시

### **3. IMAGE 패킷 구조**
- JSON 헤더와 raw JPEG data 분리
- Server 구현에 따라 수신 방식 조정 필요
- 현재는 인프라만 구축, 실제 표시는 추후 구현

---

**작성자**: Claude (Anthropic AI Assistant)  
**버전**: 1.0  
**작성일**: 2026-03-12