AnoMap C++ 리팩토링 전략 명세서 | AnoMap-REFARCH-002 v1.0.0
1 / 10
AnoMap C++ 리팩토링 전략 명세서
문서 ID: AnoMap-REFARCH-002 v1.0.0 2026-03-10
대상 범위: Qt/Back/*.cpp, Qt/Back/*.hpp, Src/**/*.cpp, Src/**/*.hpp, Core.cpp, Core.hpp
1. 현재 구조 진단
현재 코드베이스는 아래 두 계층으로 명확하게 분리되어 있다.
계층 위치 역할 현재 파일
Layer 1 Src/ 순수 C++, Qt 의존 없음 CameraStore, DeviceStore,
ServerStatusStore,
AlarmDispatcher,
NetworkService, ThreadPool
Layer 2 Qt/Back/ QObject 어댑터, QML 노출 CameraModel, DeviceModel,
ServerStatusModel, Login,
Signup, VideoManager,
VideoSurfaceItem, VideoWorker,
NetworkBridge, AlarmManager
Orchestrator 루트 의존성 주입 진입점 Core.cpp / Core.hpp, App.cpp
1.1 진단된 문제점
• 파일-클래스 1:1 강제: Login.hpp/Signup.hpp 는 공통 상태 패턴(isLoading, isError, errorMessage, m_bridge,
m_host, m_port)을 각자 보유. 코드 중복 ~70 줄.
• 기능 분산: CameraModel 이 SplitDirection enum, SlotInfo struct, CameraEntry struct, CameraModel
class 를 하나의 파일에 모두 정의하지만 이름이 'Model'로만 되어 있어 외부에서 찾기 어렵.
• Store 3 개가 동일한 패턴 반복: updateFromJson(json, cb) + mutex + snapshot(). 추상 인터페이스 없음.
• DeviceModel/ServerStatusModel 의 onStoreUpdated 루프 구조가 거의 동일 (순회-비교-dataChanged 패턴).
• NetworkBridge::connectToServer 내 콜백 등록이 80 줄짜리 단일 함수. 각 패킷 핸들러 분리 불가.
• VideoWorker/VideoSurfaceItem 가 Video.hpp 에만 의존하지만 Back/에 위치해 계층 경계 모호.
• Data 구조체(CameraData, DeviceData 등)가 개별 파일로 분산되어 include 체인이 복잡.
2. 리팩토링 원칙
2.1 파일 이름 = 추상 기능 단위
파일 이름은 '클래스 이름'이 아닌 '기능 도메인'을 나타낸다. 하나의 파일이 해당 기능에 관련된 여러
struct/class/enum을 포함할 수 있다.
파일명 패턴 포함 가능한 것 예시
Domain.hpp / .cpp 해당 도메인의 Data struct + Store
class + 관련 enum
Camera.hpp: CameraData, CameraStore,
SplitDirection, SlotInfo
파일명 패턴 포함 가능한 것 예시
Auth.hpp / .cpp 공통 베이스 +Login + Signup Auth.hpp: AuthController(base),
LoginController, SignupController
VideoStream.hpp / .cpp VideoWorker + VideoManager +
관련 타입
VideoStream.hpp: FrameData,
VideoWorker, VideoManager
IStore.hpp Store 공통 인터페이스 (template) IStore<T>: updateFromJson, snapshot
2.2 중복 제거 = 추상 베이스 또는 템플릿
3개 이상의 클래스가 동일한 패턴을 보이면 공통 베이스 또는 template을 추출한다.
• Store 3 개 → IJsonStore<T> 인터페이스 (또는 JsonStore<T> CRTP 베이스)
• Login/Signup → AuthController 공통 베이스
• Model 의 onStoreUpdated 패턴 → 가능하면 QAbstractModelBase<Entry, Data> 템플릿
2.3 Legacy 코드 처리 원칙
• 이미 동작하는 Core.cpp wiring 은 건드리지 않는다. 파일 재구성만 한다.
• Q_INVOKABLE, Q_PROPERTY, signal/slot 인터페이스는 QML 과의 계약이므로 이름을 바꾸지 않는다.
• swapCameraUrls 같은 legacy alias 는 deprecated 주석을 달고 유지한다.
3. 목표 디렉토리 구조
3.1 Src/ 계층 목표 구조
Src/
Config.hpp ← 변경 없음
Domain/
IStore.hpp ← 신규: Store 공통 인터페이스
Camera.hpp / .cpp ← 통합: CameraData + CameraStore + SplitDirection + SlotInfo
Device.hpp / .cpp ← 통합: DeviceData + DeviceCapabilityData + DeviceStore
ServerStatus.hpp / .cpp ← 통합: ServerStatusData + DeviceStatusData + ServerStatusStore
Alarm.hpp / .cpp ← 통합: AlarmEvent + AlarmDispatcher
Network/
INetworkService.hpp ← 변경 없음
NetworkService.hpp / .cpp ← 변경 없음
Session.hpp / .cpp ← 변경 없음
Header.hpp ← 변경 없음
Video.hpp / .cpp ← 변경 없음
Thread/
ThreadPool.hpp / .cpp ← 변경 없음
3.2 Qt/Back/ 계층 목표 구조
Qt/Back/
Auth.hpp / .cpp ← 신규: AuthController(base) + LoginController +
SignupController
Camera.hpp / .cpp ← 통합: CameraEntry + CameraModel
AnoMap C++ 리팩토링 전략 명세서 | AnoMap-REFARCH-002 v1.0.0
3 / 10
Device.hpp / .cpp ← 통합: DeviceEntry + DeviceCapability + DeviceModel
ServerStatus.hpp / .cpp ← 통합: DeviceStatus + ServerStatus + ServerStatusModel
VideoStream.hpp / .cpp ← 통합: VideoWorker + VideoManager
VideoSurfaceItem.hpp / .cpp ← 변경 없음 (QQuickItem, 단독 역할)
NetworkBridge.hpp / .cpp ← 변경 없음 (단일 책임 유지)
AlarmManager.hpp / .cpp ← 변경 없음 (단일 책임 유지)
3.3 삭제 대상 파일
삭제 파일 흡수되는 위치 이유
Src/Domain/CameraData.hpp Src/Domain/Camera.hpp Data 구조체만 있는 파일 통합
Src/Domain/CameraStore.hpp/.
cpp
Src/Domain/Camera.hpp/.cpp 같은 도메인
Src/Domain/DeviceData.hpp Src/Domain/Device.hpp Data 구조체만 있는 파일 통합
Src/Domain/DeviceStore.hpp/.c
pp
Src/Domain/Device.hpp/.cpp 같은 도메인
Src/Domain/ServerStatusData.h
pp
Src/Domain/ServerStatus.hpp Data 구조체만 있는 파일 통합
Src/Domain/ServerStatusStore.
hpp/.cpp
Src/Domain/ServerStatus.hpp/.cpp 같은 도메인
Src/Domain/AlarmEvent.hpp Src/Domain/Alarm.hpp AlarmDispatcher와 같은 도메인
Src/Domain/AlarmDispatcher.hp
p/.cpp
Src/Domain/Alarm.hpp/.cpp AlarmEvent와 같은 도메인
Qt/Back/Login.hpp/.cpp Qt/Back/Auth.hpp/.cpp AuthController 베이스로 통합
Qt/Back/Signup.hpp/.cpp Qt/Back/Auth.hpp/.cpp AuthController 베이스로 통합
Qt/Back/CameraModel.hpp/.cpp Qt/Back/Camera.hpp/.cpp CameraEntry와 같은 도메인
Qt/Back/DeviceModel.hpp/.cpp Qt/Back/Device.hpp/.cpp DeviceEntry와 같은 도메인
Qt/Back/ServerStatusModel.hp
p/.cpp
Qt/Back/ServerStatus.hpp/.cpp Status 구조체와 같은 도메인
Qt/Back/VideoManager.hpp/.cp
p
Qt/Back/VideoStream.hpp/.cpp VideoWorker와 같은 기능 단위
Qt/Back/VideoWorker.hpp/.cpp Qt/Back/VideoStream.hpp/.cpp VideoManager와 같은 기능 단위
4. 각 파일 상세 명세
4.1 Src/Domain/IStore.hpp — Store 공통 인터페이스
목적: CameraStore, DeviceStore, ServerStatusStore가 공유하는 updateFromJson + snapshot 패턴을
인터페이스로 표현한다.
포함 항목:
• template<typename T> class IStore — 순수 인터페이스
• virtual void updateFromJson(const std::string &json, std::function<void(T)> cb) = 0
• virtual T snapshot() const = 0
JsonStore<T, Derived> CRTP 베이스 (선택): mutex, m_data, snapshot() 기본 구현을 제공하여 각 Store가
parseJson만 override.
영향 받는 파일: Camera.hpp, Device.hpp, ServerStatus.hpp
4.2 Src/Domain/Camera.hpp / .cpp
목적: 카메라 도메인 전체를 하나의 파일로 표현. 순수 C++, Qt 의존 없음.
항목 타입 설명
CameraData struct rtspUrl, title, cameraType, isOnline (기존
CameraData.hpp 동일)
SplitDirection enum class None/Col/Row/Grid (기존 CameraModel.hpp에서 이동)
CameraStore class IStore<vector<CameraData>> 구현. updateFromJson +
snapshot (기존 CameraStore 동일)
이동: SplitDirection은 Qt 헤더(CameraModel.hpp)에 있던 것을 순수 C++ 계층으로 내린다. SlotInfo는 Qt
타입(QString)을 사용하므로 Qt/Back/Camera.hpp에 유지.
4.3 Src/Domain/Device.hpp / .cpp
포함 항목: DeviceCapabilityData (struct), DeviceData (struct), DeviceStore (class : IStore).
변경 사항 없음 — 기존 DeviceData.hpp + DeviceStore.hpp/.cpp 내용 그대로 합산.
4.4 Src/Domain/ServerStatus.hpp / .cpp
포함 항목: DeviceStatusData (struct), ServerStatusData (struct), ServerStatusStore (class : IStore).
변경 사항 없음 — 기존 ServerStatusData.hpp + ServerStatusStore.hpp/.cpp 내용 그대로 합산.
4.5 Src/Domain/Alarm.hpp / .cpp
포함 항목: AlarmEvent (struct), AlarmDispatcher (class).
변경 사항 없음 — 기존 AlarmEvent.hpp + AlarmDispatcher.hpp/.cpp 내용 그대로 합산.
4.6 Qt/Back/Auth.hpp / .cpp
목적: Login과 Signup의 공통 패턴(~70줄 중복)을 AuthController 베이스로 추출하여 제거.
AuthController (abstract QObject base):
• Q_PROPERTY: isLoading, isError, errorMessage → 공통 멤버 변수 및 emit 로직
• protected: m_bridge, m_host, m_port, m_pending* 멤버
• protected: setLoading(bool), setError(QString), clearError(), onConnected() = 0 (pure virtual)
• public: virtual void submit(...) = 0 — 각 서브클래스가 구현
LoginController (: public AuthController):
• Q_PROPERTY: state, username
• Q_INVOKABLE login(id, password)
• slots: handleLoginSuccess, handleLoginFailed, onConnected() override
• signals: loginSuccess(), logoutRequested()
AnoMap C++ 리팩토링 전략 명세서 | AnoMap-REFARCH-002 v1.0.0
5 / 10
SignupController (: public AuthController):
• Q_INVOKABLE signup(id, email, password)
• slots: handleSignupSuccess, handleSignupFailed, onConnected() override
• signals: signupSuccess(QString)
중복 제거 효과: setLoading/setError/clearError/m_bridge/m_host/m_port 각 50~70줄 × 2 = ~120줄 제거.
QML 인터페이스 호환: loginController, signupController property 이름 유지. Core.cpp는 LoginController*,
SignupController*로 교체.
4.7 Qt/Back/Camera.hpp / .cpp
포함 항목:
• SlotInfo (struct + Q_DECLARE_METATYPE) — 기존 CameraModel.hpp 에서 이동
• CameraEntry (struct) — 기존 CameraModel.hpp 에서 이동
• CameraModel (class : QAbstractListModel) — 기존과 동일, include 경로 수정
SplitDirection은 Src/Domain/Camera.hpp로 이동했으므로 이 파일에서는 #include
"../../Src/Domain/Camera.hpp"로 참조.
영향: Core.cpp의 #include "Qt/Back/CameraModel.hpp" → #include "Qt/Back/Camera.hpp"
4.8 Qt/Back/Device.hpp / .cpp
포함 항목: DeviceCapability (struct, Qt 레이어 뷰), DeviceEntry (struct), DeviceModel (class : QAbstractListModel).
변경 사항: 기존 DeviceModel.hpp/.cpp 내용 그대로, 파일 이름만 변경. include 경로 수정.
4.9 Qt/Back/ServerStatus.hpp / .cpp
포함 항목: DeviceStatus (struct, Qt 레이어 뷰), ServerStatus (struct), ServerStatusModel (class : QObject).
변경 사항: 기존 ServerStatusModel.hpp/.cpp 내용 그대로, 파일 이름만 변경.
4.10 Qt/Back/VideoStream.hpp / .cpp
목적: VideoWorker + VideoManager는 RTSP 스트리밍이라는 하나의 기능 단위이다. 파일을 분리할 이유가 없다.
포함 항목:
• VideoWorker (class : QObject) — 기존 그대로, FrameData struct 포함
• VideoManager (class : QObject) — 기존 그대로
주의: VideoWorker가 먼저 선언되어야 VideoManager가 참조 가능. 파일 내 선언 순서: VideoWorker →
VideoManager.
App.cpp: qmlRegisterType<VideoSurfaceItem>는 그대로. VideoSurfaceItem.hpp는 VideoStream.hpp를 include.
5. 핵심 추상화 코드 스케치
5.1 IStore<T> 인터페이스
#pragma once
#include <functional>
#include <string>
template <typename T>
class IStore {
public:
using Callback = std::function<void(T)>;
virtual ~IStore() = default;
virtual void updateFromJson(const std::string &json, Callback cb) = 0;
virtual T snapshot() const = 0;
};
5.2 JsonStore<T> CRTP 베이스 (선택적 적용)
각 Store가 parseJson 하나만 구현하도록 mutex/m_data/snapshot을 CRTP로 공유.
template <typename T, typename Derived>
class JsonStore : public IStore<T> {
public:
void updateFromJson(const std::string &json, Callback cb) override {
T result = static_cast<Derived*>(this)->parseJson(json);
{ std::lock_guard lk(m_mutex); m_data = result; }
if (cb) cb(std::move(result));
}
T snapshot() const override {
std::lock_guard lk(m_mutex);
return m_data;
}
protected:
mutable std::mutex m_mutex;
T m_data;
};
각 Store 구현:
class CameraStore : public JsonStore<std::vector<CameraData>, CameraStore> {
public:
std::vector<CameraData> parseJson(const std::string &json); // 기존 parse 로직
};
5.3 AuthController 공통 베이스
class AuthController : public QObject {
Q_OBJECT
Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
Q_PROPERTY(bool isError READ isError NOTIFY isErrorChanged)
Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
public:
explicit AuthController(NetworkBridge *bridge,
const QString &host,
const QString &port,
QObject *parent = nullptr);
bool isLoading() const { return m_isLoading; }
AnoMap C++ 리팩토링 전략 명세서 | AnoMap-REFARCH-002 v1.0.0
7 / 10
bool isError() const { return m_isError; }
QString errorMessage() const { return m_errorMessage; }
signals:
void isLoadingChanged();
void isErrorChanged();
void errorMessageChanged();
protected:
virtual void onConnected() = 0; // 각 서브클래스 구현
void setLoading(bool v);
void setError(const QString &msg);
void clearError();
NetworkBridge *m_bridge;
QString m_host, m_port;
bool m_isLoading{false}, m_isError{false};
QString m_errorMessage;
};
LoginController와 SignupController는 AuthController를 상속하며, login()/signup() Q_INVOKABLE과 각자의
handle* slot만 추가로 구현한다.
6. include 경로 변경 일람
리팩토링 후 변경이 필요한 #include 구문 전체 목록.
파일 변경 전 변경 후 비고
Core.cpp #include
Qt/Back/CameraModel.hpp
#include Qt/Back/Camera.hpp
Core.cpp #include
Qt/Back/DeviceModel.hpp
#include Qt/Back/Device.hpp
Core.cpp #include
Qt/Back/ServerStatusModel.hpp
#include
Qt/Back/ServerStatus.hpp
Core.cpp #include Qt/Back/Login.hpp #include Qt/Back/Auth.hpp
Core.cpp #include Qt/Back/Signup.hpp 제거 (Auth.hpp에 포함)
Core.cpp #include
Qt/Back/VideoManager.hpp
#include
Qt/Back/VideoStream.hpp
Core.cpp #include
Qt/Back/VideoWorker.hpp (없음)
VideoStream.hpp에 포함됨
Core.hpp class CameraModel; class CameraModel; (그대로) 전방선언만
파일 변경 전 변경 후 비고
Core.hpp class Login; class Signup; class LoginController; class
SignupController;
전방선언
이름 변경
Core.hpp class VideoManager; class VideoManager; (그대로) 전방선언만
Qt/Back/Camera.hpp #include CameraData.hpp (제거) #include
../../Src/Domain/Camera.hpp
SplitDirecti
on 포함
Qt/Back/Device.hpp #include DeviceData.hpp (제거) #include
../../Src/Domain/Device.hpp
Qt/Back/ServerStatus.h
pp
#include ServerStatusData.hpp
(제거)
#include
../../Src/Domain/ServerStatus.hpp
Qt/Back/Auth.hpp 신규 #include NetworkBridge.hpp
Qt/Back/VideoStream.h
pp
신규 #include
../../Src/Network/Video.hpp
App.cpp #include
Qt/Back/VideoSurfaceItem.hpp
그대로
VideoSurfaceItem.hpp #include VideoWorker.hpp #include VideoStream.hpp VideoWork
er 포함됨
7. CMakeLists.txt 변경 사항
7.1 Src/CMakeLists.txt
삭제할 소스:
• Domain/CameraData.hpp (헤더 전용, target_sources 등록 없음)
• Domain/CameraStore.hpp / CameraStore.cpp
• Domain/DeviceData.hpp / DeviceStore.hpp / DeviceStore.cpp
• Domain/ServerStatusData.hpp / ServerStatusStore.hpp / ServerStatusStore.cpp
• Domain/AlarmEvent.hpp / AlarmDispatcher.hpp / AlarmDispatcher.cpp
추가할 소스:
• Domain/Camera.hpp / Camera.cpp
• Domain/Device.hpp / Device.cpp
• Domain/ServerStatus.hpp / ServerStatus.cpp
• Domain/Alarm.hpp / Alarm.cpp
• Domain/IStore.hpp (헤더 전용)
7.2 Qt/Back/CMakeLists.txt
삭제할 소스:
• Login.hpp / Login.cpp
• Signup.hpp / Signup.cpp
• CameraModel.hpp / CameraModel.cpp
• DeviceModel.hpp / DeviceModel.cpp
• ServerStatusModel.hpp / ServerStatusModel.cpp
• VideoManager.hpp / VideoManager.cpp
AnoMap C++ 리팩토링 전략 명세서 | AnoMap-REFARCH-002 v1.0.0
9 / 10
• VideoWorker.hpp / VideoWorker.cpp
추가할 소스:
• Auth.hpp / Auth.cpp
• Camera.hpp / Camera.cpp
• Device.hpp / Device.cpp
• ServerStatus.hpp / ServerStatus.cpp
• VideoStream.hpp / VideoStream.cpp
8. 구현 순서 (권장)
각 단계는 독립적으로 빌드 검증 가능하다. 단계를 건너뛰지 말 것.
단계 작업 변경 파일 검증 방법
1 IStore.hpp 생성 Src/Domain/IStore.hpp (신규) 컴파일 확인
2 Src Domain 파일 통합
(Alarm 먼저)
Alarm.hpp/.cpp 생성 → 기존
AlarmEvent.hpp,
AlarmDispatcher.hpp/.cpp 삭제
Core.cpp include 수정 후 빌드
3 Camera/Device/ServerStat
us Domain 통합
Camera.hpp/.cpp 등 3쌍 생성 →
기존 6개 파일 삭제
빌드
4 Qt/Back Camera.hpp/.cpp
생성
CameraModel 내용 이동 +
CameraModel.hpp 삭제
CameraModel QML 동작 확인
5 Qt/Back Device.hpp/.cpp
생성
DeviceModel 내용 이동 +
DeviceModel.hpp 삭제
DeviceModel QML 동작 확인
6 Qt/Back
ServerStatus.hpp/.cpp
생성
ServerStatusModel 내용 이동 serverStatus QML 동작 확인
7 Qt/Back Auth.hpp/.cpp
생성
AuthController 베이스 +
LoginController + SignupController
로그인/회원가입 동작 확인
8 Qt/Back
VideoStream.hpp/.cpp
생성
VideoWorker + VideoManager 합산
→ 기존 4파일 삭제
영상 스트리밍 동작 확인
9 CMakeLists.txt 정리 Qt/Back + Src CMakeLists 파일 목록
갱신
클린 빌드
10 Core.cpp/hpp include 및
타입 이름 정리
Login* → LoginController*, Signup*
→ SignupController*
전체 빌드 + 런타임 검증
8.1 단계별 위험도
단계 위험도 이유 완화 방법
1~3 낮음 순수 C++ 파일 이동,
Qt 없음
파일 내용 복사 후 include 경로만 수정
4~6 낮음 파일 이름 변경, 로직
변경 없음
grep으로 include 잔재 확인
단계 위험도 이유 완화 방법
7 중간 AuthController 신규
베이스 클래스, Qt
MOC 관여
QML에서 loginController/signupController
property 동작 테스트 필수
8 낮음 파일 이름만 변경,
로직 동일
QTimer 구동 영상 정상 여부 확인
9~10 낮음 기계적 작업 클린 빌드로 누락 없음 확인
9. 변경하지 않는 것
아래 항목은 이번 리팩토링 범위 밖이다. 건드리지 않는다.
항목 이유
VideoSurfaceItem.hpp/.cpp QQuickItem 단독 책임, 분리 근거 없음. QTimer 최적화 완료.
NetworkBridge.hpp/.cpp 단일 책임 완수 (Qt 타입 브릿지). 분리 불필요.
AlarmManager.hpp/.cpp 단일 책임 완수 (AlarmEvent → Qt signal). 분리 불필요.
Core.cpp wireSignals() 로직 신호 배선은 그대로. include/타입 이름만 변경.
App.cpp qmlRegisterType 등록 코드 그대로. 최소 변경.
Src/Network/ 전체 Session, NetworkService, Video는 독립 기능. 통합 대상 아님.
Src/Thread/ThreadPool 범용 유틸리티. 도메인 통합과 무관.
Src/Config.hpp 배포 상수. 변경 불필요.
Q_PROPERTY / Q_INVOKABLE
이름
QML과의 계약. 이름 변경 시 QML 전수 수정 필요.
QML context property 이름 loginController, signupController, videoManager 등 QML에서 직접 참조.
10. 완료 기준 (Definition of Done)
1. cmake --build 클린 빌드 성공 (warning 증가 없음)
2. 로그인 → 대시보드 → 카메라 스트리밍 → 로그아웃 전체 흐름 정상
3. 이전 파일(CameraModel.hpp 등) 잔재 include 가 프로젝트 어디에도 없음 (grep 확인)
4. QML Profiler GUI Thread Sync 수치가 이전 대비 동등 이하 유지
5. AuthController 베이스 도입 후 Login/Signup 중복 코드 ~120 줄 제거 확인
6. IStore 인터페이스 적용 후 각 Store 가 parseJson 하나만 구현함을 코드 리뷰 확인
본 명세서는 AnoMap-REFARCH-002 v1.0.0으로 등록된다.