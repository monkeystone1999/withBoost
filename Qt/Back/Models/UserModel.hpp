#pragma once
#include "../../../Src/Domain/StatusManager.hpp"
#include <QAbstractListModel>
/**
 * @file UserModel.hpp
 * @brief 사용자 계정 정보 및 세션 상태 관리 모델
 *
 * 이 파일은 시스템 서비스에 등록된 사용자 목록과 각 사용자의 권한(Role),
 * 실시간 접속 상태 및 마지막 활동 로그를 관리하는 모델을 정의합니다.
 */

// UserData structure removed since we pull directly from StatusManager

// UserEntry removed. Using UserMeta from StatusManager.hpp directly.

/**
 * @class UserModel
 * @brief 실시간 사용자 현황 및 권한 정보를 UI에 공급하는 QAbstractListModel
 * 서브클래스
 *
 * **Standard Usage Methodology:**
 * 1. 인증 서버 또는 코어 스토어로부터 UserData 벡터를 수신하여
 * onStoreUpdated()로 모델을 전면 갱신합니다.
 * 2. 특정 사용자 행위 시 isUserAdmin()을 통해 관리자 전용 기능(설정 변경 등)의
 * 접근 권한을 검증합니다.
 * 3. count 프로퍼티를 QML 상단 바 등에 바인딩하여 현재 총 등록 사용자 수를
 * 표시합니다.
 */
class UserModel : public QAbstractListModel {
  Q_OBJECT
  /** @brief 현재 목록에 포함된 총 사용자 수 */
  Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
  /** @brief QML 레이어 데이터 바인딩을 위한 역할 정의 */
  enum Roles {
    UserIdRole = Qt::UserRole + 1, /**< 고유 아이디 */
    UsernameRole,                  /**< 이름 */
    EmailRole,                     /**< 이메일 */
    RoleRole,                      /**< 역할 (admin/user) */
    IsOnlineRole,                  /**< 온라인 여부 */
    LastLoginRole,                 /**< 마지막 로그인 일시 */
    IpAddressRole,                 /**< 접속 IP */
    ActiveCamerasRole              /**< 사용 중인 카메라 대수 */
  };

  explicit UserModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const { return users_.count(); }
  /** @brief 전체 사용자 목록 초기화 */
  Q_INVOKABLE void clearAll();

  /** @brief UID를 통한 사용자 이름 조회 */
  Q_INVOKABLE QString getUsernameById(const QString &userId) const;
  /** @brief 특정 사용자의 관리자 권한 여부 확인 */
  Q_INVOKABLE bool isUserAdmin(const QString &userId) const;

  void setStatusManager(StatusManager *mgr) { statusManager_ = mgr; }

public slots:
  /** @brief 서버 데이터 스냅샷을 통한 모델 동기화 */
  void refreshFromStatusManager();

signals:
  /** @brief 사용자 수 변경 시 발생 */
  void countChanged();

private:
  int findIndexByUserId(const QString &userId) const;

  StatusManager *statusManager_ = nullptr;
  QList<UserMeta> users_;
  QHash<QString, int> byId_; /**< 빠른 조회를 위한 ID-인덱스 맵 */
};

/**
 * @section Workflow Guide
 *
 * **[UserModel 계정 관리 워크플로우]**
 *
 * 1. 데이터 동기화: 백엔드 서비스(AuthService 등)에서 사용자 명단이 변경되면
 * `onStoreUpdated`가 호출되어 전체 UI 리스트를 갱신합니다.
 * 2. 권한 필터링: 사용자 관리 페이지(UserManagement.qml) 등에서 특정 사용자
 * 선택 시 `isUserAdmin` 결과를 바탕으로 '삭제' 또는 '권한 변경' 버튼의 활성화
 * 상태를 결정합니다.
 * 3. 세션 모니터링: `IsOnlineRole`의 변화는 UI 리스트의 상태
 * 인디케이터(Green/Gray 점)에 즉각 반영되어 관리자가 현재 동시 접속자 현황을
 * 파악할 수 있게 합니다.
 */
