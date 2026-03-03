#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QFontDatabase>
#include <QQmlContext>
#include <QtQml/qqmlextensionplugin.h>
#include <QWKQuick/qwkquickglobal.h>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlEngine>
#include "Qt/Back/CameraModel.hpp"
#include "Src/Network/Session.hpp"

// FrontEndplugin_AnoMap_frontPlugin.cpp 에서 생성된 플러그인 클래스 강제 링크
Q_IMPORT_QML_PLUGIN(AnoMap_frontPlugin)

// ── QML 파일 경로 오타(views/ -> View/) 동적 교정용 인터셉터 ──
class PathCaseInterceptor : public QQmlAbstractUrlInterceptor {
public:
    QUrl intercept(const QUrl &path, DataType type) override {
        QString urlStr = path.toString();
        // CMakeLists.txt나 QML 파일을 직접 수정하지 않고 C++에서 경로를 보정합니다.
        if (urlStr.contains("/views/")) {
            urlStr.replace("/views/", "/View/");
        }
        return QUrl(urlStr);
    }
};

void FrontInit(QQmlApplicationEngine& engine);

int main(int argc, char** argv){
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // URL 인터셉터 등록
    PathCaseInterceptor interceptor;
    engine.setUrlInterceptor(&interceptor);

    // QWindowKit QML 타입 수동 등록 (필수)
    QWK::registerTypes(&engine);

    // C++ 예외 시그널 연결...
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    // 강제 리소스 초기화: 정적 빌드 시 리소스 로더가 파일들을 못 찾는 현상 방지
    Q_INIT_RESOURCE(qmake_AnoMap_front);
    Q_INIT_RESOURCE(FrontEnd_raw_qml_0);
    Q_INIT_RESOURCE(assets); // Qt/Front/Assets 의 리소스 강제 로드

    // ── Theme 싱글톤 수동 등록 ────────────────────────────────────────────────
    // CMakeLists.txt의 프로퍼티 대소문자 오타(theme/ vs Theme/)를
    // CMake 수정 없이 C++에서 수동으로 싱글톤 등록하여 해결합니다.
    qmlRegisterSingletonType(QUrl("qrc:/qt/qml/AnoMap/front/Theme/Theme.qml"), "AnoMap.front", 1, 0, "Theme");
    qmlRegisterSingletonType(QUrl("qrc:/qt/qml/AnoMap/front/Theme/Icon.qml"), "AnoMap.front", 1, 0, "Icon");
    qmlRegisterSingletonType(QUrl("qrc:/qt/qml/AnoMap/front/Theme/Typography.qml"), "AnoMap.front", 1, 0, "Typography");

    // ── CameraModel 등록 및 컨텍스트 프로퍼티 설정 ─────────────────────────
    CameraModel* cameraModel = new CameraModel(&engine);
    cameraModel->addCamera("Camera 01", "rtsp://192.168.1.1/stream1", true);
    cameraModel->addCamera("Camera 02", "rtsp://192.168.1.2/stream2", true);
    cameraModel->addCamera("Camera 03", "rtsp://192.168.1.3/stream3", false);
    cameraModel->addCamera("Camera 04", "rtsp://192.168.1.4/stream4", true);
    engine.rootContext()->setContextProperty("cameraModel", cameraModel);

    FrontInit(engine);
    
    // static QML 모듈 리소스 경로 매핑
    engine.addImportPath("qrc:/qt/qml");
    engine.addImportPath(":/qt/qml");
    engine.addImportPath("qrc:/");

    engine.loadFromModule("AnoMap.front", "Main");

    SessionManager manager;
    return app.exec();
}

void FrontInit(QQmlApplicationEngine& engine){
    QFontDatabase::addApplicationFont(":/Core/Fonts/01HanwhaB.ttf");
    QFontDatabase::addApplicationFont(":/Core/Fonts/02HanwhaR.ttf");
    QFontDatabase::addApplicationFont(":/Core/Fonts/03HanwhaL.ttf");
    QFontDatabase::addApplicationFont(":/Core/Fonts/04HanwhaGothicB.ttf");
    QFontDatabase::addApplicationFont(":/Core/Fonts/05HanwhaGothicR.ttf");
    QFontDatabase::addApplicationFont(":/Core/Fonts/06HanwhaGothicL.ttf");
    QFontDatabase::addApplicationFont(":/Core/Fonts/07HanwhaGothicEL.ttf");
    QFontDatabase::addApplicationFont(":/Core/Fonts/08HanwhaGothicT.ttf");
}
