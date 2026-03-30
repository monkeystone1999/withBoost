#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

// ============================================================
//  SettingsController — 앱 설정 영속화 (QSettings 기반)
//
//  이전에 Main.qml OptionDialog에 TODO로 남아있던:
//    - darkMode / alarmSound / defaultGrid / logLevel
//  를 C++에서 load/save.
//
//  Theme.isDark ↔ settingsController.darkMode 양방향 바인딩.
// ============================================================
/**
 * @file SettingsController.hpp
 * @brief 애플리케이션 설정 영속화 및 관리 컨트롤러
 *
 * 이 파일은 사용자 환경 설정(테마, 알림, 그리드 등)을 QSettings 및 외부 JSON
 * 파일로부터 로드하고 저장하며, QML UI와 양방향 바인딩을 제공하는 컨트롤러를
 * 정의합니다.
 */

/**
 * @class SettingsController
 * @brief 사용자 기본 설정 및 시스템 구성 파라미터 관리 클래스
 *
 * **Standard Usage Methodology:**
 * 1. 생성자에서 load()가 실행되어 시스템의 이전 상태를 자동으로 복원합니다.
 * 2. QML 설정 창(OptionDialog)의 컨트롤들과 각 프로퍼티를 바인딩하여 실시간
 * 수정을 반영합니다.
 * 3. 앱 종료 전 또는 사용자의 저장 요청 시 save()를 호출하여 설정값을
 * 영속화합니다.
 */
class SettingsController : public QObject {
  Q_OBJECT
  /** @brief 다크 모드 활성화 여부 (Theme.isDark와 연동) */
  Q_PROPERTY(
      bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
  /** @brief 알림 발생 시 효과음 재생 여부 */
  Q_PROPERTY(bool alarmSound READ alarmSound WRITE setAlarmSound NOTIFY
                 alarmSoundChanged)
  /** @brief 기본 레이아웃 그리드 수 (예: 2, 4, 9) */
  Q_PROPERTY(int defaultGrid READ defaultGrid WRITE setDefaultGrid NOTIFY
                 defaultGridChanged)
  /** @brief 시스템 로그 기록 수준 (DEBUG, INFO, ERROR 등) */
  Q_PROPERTY(
      QString logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)
  /** @brief 시스템 지원 해상도 리스트 (Settings.json 로드) */
  Q_PROPERTY(QStringList resolutions READ resolutions NOTIFY resolutionsChanged)
  /** @brief 기본 디스플레이 해상도 설정 */
  Q_PROPERTY(QString defaultResolution READ defaultResolution WRITE
                 setDefaultResolution NOTIFY defaultResolutionChanged)
  /** @brief 연결 대상 서버 IP 주소 (Settings.json) */
  Q_PROPERTY(QString serverIp READ serverIp NOTIFY serverIpChanged)
  /** @brief 최대 프레임 레이트 제한 설정 (Settings.json) */
  Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)

public:
  explicit SettingsController(QObject *parent = nullptr);

  bool darkMode() const { return darkMode_; }
  bool alarmSound() const { return alarmSound_; }
  int defaultGrid() const { return defaultGrid_; }
  QString logLevel() const { return logLevel_; }
  QStringList resolutions() const { return resolutions_; }
  QString defaultResolution() const { return defaultResolution_; }
  QString serverIp() const { return serverIp_; }
  int fps() const { return fps_; }

  void setDarkMode(bool v);
  void setAlarmSound(bool v);
  void setDefaultGrid(int v);
  void setLogLevel(const QString &v);
  void setDefaultResolution(const QString &v);

  /** @brief 현재 프로퍼티 상태를 QSettings(레지스트리/INI)에 저장 */
  Q_INVOKABLE void save();

  /** @brief QSettings 및 Settings.json으로부터 설정값 로드 */
  Q_INVOKABLE void load();

signals:
  void darkModeChanged();
  void alarmSoundChanged();
  void defaultGridChanged();
  void logLevelChanged();
  void resolutionsChanged();
  void defaultResolutionChanged();
  void serverIpChanged();
  void fpsChanged();

private:
  bool darkMode_{true};              /**< 테마 상태 */
  bool alarmSound_{true};            /**< 사운드 활성화 상태 */
  int defaultGrid_{4};               /**< 그리드 분할 수 (기본 2x2) */
  QString logLevel_{"INFO"};         /**< 현재 로그 레벨 */
  QStringList resolutions_;          /**< 파싱된 해상도 목록 */
  QString defaultResolution_;        /**< 선택된 기본 해상도 */
  QString serverIp_{"192.168.0.58"}; /**< 서버 IP 호스트 */
  int fps_{30};                      /**< 타겟 FPS */
};

/**
 * @section Workflow Guide
 *
 * **[SettingsController 구성 관리 워크플로우]**
 *
 * 1. 로드: `load()`가 실행되면 하이브리드 로딩이 수행됩니다.
 *    - `QSettings`: 런타임 사용자 변경값 (테마, 그리드 등)
 *    - `Settings.json`: 배포 시 확정된 시스템 고정값 (ServerIP, Support
 * Resolutions)
 * 2. 갱신: 유저가 UI 다이얼로그에서 스위치를 조작하면 해당 프로퍼티의 `set`
 * 메서드가 실행됩니다.
 * 3. 영속화: 유저가 'Save'를 누르거나 앱이 꺼지면 `save()`를 통해 `QSettings`
 * 영역에 반영됩니다.
 */
