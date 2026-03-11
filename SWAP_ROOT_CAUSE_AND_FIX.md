# Swap 완전 분석 및 수정 명세

> 작성일: 2026-03-10  
> 방법: 코드의 실제 실행 순서를 한 줄씩 추적하여 오염 지점 확정

---

## 진짜 문제: 타이밍 역전 (Timing Inversion)

### 실제 실행 순서 추적

`cameraModel.swapSlots(indexA, indexB)` 가 호출되면 C++ 에서 동기적으로 다음 순서로 실행된다.

```
① std::swap(a.rtspUrl, b.rtspUrl)     ← 데이터 교환 완료
② emit dataChanged(indexA, {RtspUrlRole, IsOnlineRole, ...})
   └→ Qt 바인딩 엔진이 즉시 동기적으로 전파
      └→ delegate[A].model.rtspUrl 갱신
         └→ CameraCard_A.rtspUrl 갱신
            └→ VideoSurface_A.rtspUrl 갱신
               └→ onRtspUrlChanged 발화
                  └→ _tryAttach() 호출
                     └→ getWorkerBySlot(slotId_A)
                        └→ m_slotToUrl[slotId_A] = "rtsp://A"  ← 아직 swap 전!
                           └→ workerA 반환  ← 잘못된 worker

③ emit dataChanged(indexB, {RtspUrlRole, IsOnlineRole, ...})
   └→ 동일하게 VideoSurface_B 도 swap 전 worker 에 연결됨

④ _emitSlotsUpdated()
   └→ emit slotsUpdated([{slotId_A, "rtsp://B"}, {slotId_B, "rtsp://A"}])
      └→ VideoManager::registerSlots()
         └→ m_slotToUrl 갱신  ← 너무 늦음. ②③에서 이미 잘못된 worker 할당됨
         └→ 신규 URL 없음 → workerRegistered emit 없음
            └→ VideoSurface 는 재연결 기회 없음
```

**결론**: `dataChanged` emit → QML 바인딩 즉시 전파 → `_tryAttach()` 호출  
이 모든 것이 `_emitSlotsUpdated()` (= m_slotToUrl 갱신) **보다 먼저** 실행된다.  
VideoSurface 가 `_tryAttach()` 를 실행하는 시점에 `m_slotToUrl` 은 아직 swap 전 상태이다.

---

## 오염 지점 목록

### 오염 1: `_tryAttach` 실행 시점에 `m_slotToUrl` 미갱신 (핵심)

위 타이밍 역전 설명 참조.

### 오염 2: `_onRegistered` 의 `slotId >= 0` 조건이 항상 true

```qml
// VideoSurface.qml
function _onRegistered(url) {
    if (root.slotId >= 0 || root.rtspUrl === url) {   // ← slotId >= 0 은 항상 true
        _tryAttach();
    }
}
```

`_tryAttach()` 실패 시 VideoSurface 는 `workerRegistered` 시그널을 대기한다.  
어떤 URL 이든 `workerRegistered` 가 emit 되면 13개 카메라의 모든 VideoSurface 가  
`_onRegistered` 에서 `_tryAttach()` 를 동시에 호출한다.

swap 후 `_emitSlotsUpdated()` → `registerSlots()` 가 신규 URL 을 찾으면  
`workerRegistered(url)` emit → 전체 VideoSurface 가 일제히 재연결 시도.  
이것이 "예측 불가능한 타이밍에 스트리밍이 됨" 현상의 원인이다.

### 오염 3: `Loader.active` 전환이 VideoSurface 를 파괴/재생성하면서 타이밍 꼬임 가중

```qml
// CameraCard.qml
Loader {
    active: root.isOnline && root.rtspUrl !== ""
```

swap 에서 `isOnline` 도 교환된다. 만약 A(online=true) ↔ B(online=true) swap 이면  
`isOnline` 이 바뀌지 않으므로 Loader 는 그대로 유지된다.  

그러나 타이밍 역전 문제(`오염 1`)로 인해 `onRtspUrlChanged` 시점에  
이미 잘못된 worker 에 연결되고, 이후 m_slotToUrl 갱신이 되어도  
VideoSurface 에 재연결 기회가 없어 화면이 멈춘다.

---

## 해결 방법: `dataChanged` 와 `_emitSlotsUpdated` 의 실행 순서 역전

### 핵심 원칙

**`m_slotToUrl` 이 갱신된 후에 QML 에 `dataChanged` 를 통보해야 한다.**

즉, `swapSlots` 에서:
1. 먼저 `_emitSlotsUpdated()` 로 `m_slotToUrl` 갱신
2. 그 다음 `emit dataChanged(...)` 로 QML 에 통보

이렇게 하면 QML 바인딩이 즉시 전파되어 `_tryAttach()` 가 호출될 때  
이미 `m_slotToUrl` 이 올바른 상태이므로 올바른 worker 를 반환한다.

---

## 수정 1: `CameraModel::swapSlots` — 실행 순서 역전

### 파일: `Qt/Back/CameraModel.cpp`

### 변경 전
```cpp
void CameraModel::swapSlots(int indexA, int indexB) {
    if (indexA < 0 || indexB < 0 || indexA >= m_cameras.size() ||
        indexB >= m_cameras.size() || indexA == indexB)
        return;

    auto &a = m_cameras[indexA];
    auto &b = m_cameras[indexB];
    std::swap(a.rtspUrl,     b.rtspUrl);
    std::swap(a.title,       b.title);
    std::swap(a.cameraType,  b.cameraType);
    std::swap(a.isOnline,    b.isOnline);
    std::swap(a.description, b.description);

    const auto idxA = createIndex(indexA, 0);
    const auto idxB = createIndex(indexB, 0);
    const QList<int> roles = {TitleRole, RtspUrlRole, IsOnlineRole,
                              DescriptionRole, CameraTypeRole};
    emit dataChanged(idxA, idxA, roles);
    emit dataChanged(idxB, idxB, roles);

    _emitSlotsUpdated();    // ← 마지막에 실행: 이미 늦음
}
```

### 변경 후
```cpp
void CameraModel::swapSlots(int indexA, int indexB) {
    if (indexA < 0 || indexB < 0 || indexA >= m_cameras.size() ||
        indexB >= m_cameras.size() || indexA == indexB)
        return;

    auto &a = m_cameras[indexA];
    auto &b = m_cameras[indexB];
    std::swap(a.rtspUrl,     b.rtspUrl);
    std::swap(a.title,       b.title);
    std::swap(a.cameraType,  b.cameraType);
    std::swap(a.isOnline,    b.isOnline);
    std::swap(a.description, b.description);

    // ── 핵심: m_slotToUrl 을 먼저 갱신하고, 그 후 QML 에 통보한다 ──────────
    // dataChanged emit 시 QML 바인딩이 즉시 동기적으로 전파되어
    // VideoSurface._tryAttach() 가 호출된다.
    // _tryAttach() 내부에서 getWorkerBySlot() 을 호출할 때
    // m_slotToUrl 이 이미 올바른 상태여야 한다.
    _emitSlotsUpdated();    // ← 먼저 실행: m_slotToUrl 갱신

    const auto idxA = createIndex(indexA, 0);
    const auto idxB = createIndex(indexB, 0);
    const QList<int> roles = {TitleRole, RtspUrlRole, IsOnlineRole,
                              DescriptionRole, CameraTypeRole};
    emit dataChanged(idxA, idxA, roles);    // ← 그 다음 QML 통보
    emit dataChanged(idxB, idxB, roles);
}
```

---

## 수정 2: `VideoSurface._onRegistered` — 과도한 재연결 방지

### 파일: `Qt/Front/Component/VideoSurface.qml`

### 변경 전
```qml
function _onRegistered(url) {
    if (root.slotId >= 0 || root.rtspUrl === url) {  // slotId>=0 이 항상 true
        try { videoManager.workerRegistered.disconnect(_onRegistered); } catch (e) {}
        _tryAttach();
    }
}
```

### 변경 후
```qml
function _onRegistered(url) {
    // slotId 기반: 자신의 슬롯에 매핑된 URL 이 등록될 때만 재연결
    // URL 기반(fallback): 자신의 rtspUrl 과 일치할 때만 재연결
    var myUrl = (root.slotId >= 0) ? videoManager.slotUrl(root.slotId) : root.rtspUrl;
    if (myUrl === url || root.rtspUrl === url) {
        try { videoManager.workerRegistered.disconnect(_onRegistered); } catch (e) {}
        _tryAttach();
    }
}
```

`videoManager.slotUrl(slotId)` 는 이미 `VideoManager::slotUrl(int)` 로 구현되어 있고  
`Q_INVOKABLE` 이므로 QML 에서 직접 호출 가능하다.

이렇게 하면 swap 후 자신의 슬롯에 해당하는 URL 의 worker 가 등록될 때만  
해당 VideoSurface 가 재연결 시도를 한다. 다른 슬롯의 `workerRegistered` 에는 반응하지 않는다.

---

## 수정 3: `VideoSurface.onRtspUrlChanged` — swap 후 즉시 재연결

### 현재 코드 (이미 수정된 상태)
```qml
onRtspUrlChanged: {
    try { videoManager.workerRegistered.disconnect(_onRegistered); } catch (e) {}
    _tryAttach();
}
```

수정 1 (순서 역전) 이 적용되면, `onRtspUrlChanged` 가 발화할 때  
이미 `m_slotToUrl` 이 갱신된 상태이므로 `_tryAttach()` 가 올바른 worker 를 반환한다.  
이 파일은 **추가 변경 불필요**하다.

단, `_tryAttach()` 내부 로직은 다음과 같이 동작해야 한다:

```qml
function _tryAttach() {
    var w = null;
    if (root.slotId >= 0) {
        w = videoManager.getWorkerBySlot(root.slotId);
        // slotId 로 찾고, 없으면 rtspUrl 로 폴백
        if (!w && root.rtspUrl !== "") {
            w = videoManager.getWorker(root.rtspUrl);
        }
    } else if (root.rtspUrl !== "") {
        w = videoManager.getWorker(root.rtspUrl);
    }

    if (w) {
        root.worker = w;          // 올바른 worker 연결
    } else {
        root.worker = null;
        // worker 가 아직 없으면 등록 대기
        try { videoManager.workerRegistered.disconnect(_onRegistered); } catch (e) {}
        videoManager.workerRegistered.connect(_onRegistered);
    }
}
```

수정 1 이 적용된 후에는 swap 시 `getWorkerBySlot(slotId)` 가  
올바른 URL → 올바른 worker 를 반환하므로 대기 경로로 진입하지 않는다.

---

## 수정 정리: 변경 파일과 줄 수

| # | 파일 | 변경 내용 | 변경 줄 수 |
|---|------|----------|-----------|
| **1** | `Qt/Back/CameraModel.cpp` | `swapSlots`: `_emitSlotsUpdated()` 를 `dataChanged` emit **앞으로** 이동 | 순서 교환 2줄 |
| **2** | `Qt/Front/Component/VideoSurface.qml` | `_onRegistered`: `slotId >= 0` 무조건 분기 → `slotUrl` 비교로 교체 | 3줄 |

**변경 없는 파일**:
- `VideoSurface.qml` 의 `onRtspUrlChanged` — 수정 1 적용 후 올바르게 동작
- `VideoManager.cpp` — 로직 올바름, 순서만 의존
- `DashboardPage.qml` — `ghost.Drag` 수동 제어 이미 적용됨
- `DashboardLayout.qml` — `reuseItems: false` 이미 적용됨

---

## 수정 후 완전한 실행 순서

```
cameraModel.swapSlots(0, 1) 호출

① std::swap(rtspUrl, title, cameraType, isOnline, description)
   m_cameras[0]: slotId=5, rtspUrl="rtsp://B"
   m_cameras[1]: slotId=7, rtspUrl="rtsp://A"

② _emitSlotsUpdated()  ← 먼저 실행
   → slotsUpdated([{5,"rtsp://B"},{7,"rtsp://A"}]) emit
   → VideoManager::registerSlots()
      → m_slotToUrl = { 5→"rtsp://B", 7→"rtsp://A" }  ✅ 갱신 완료
      → 신규 URL 없음 → workerRegistered emit 없음

③ emit dataChanged(index=0, {RtspUrlRole, ...})
   → CameraCard_0.rtspUrl = "rtsp://B"  (바인딩 즉시 전파)
   → VideoSurface_0.onRtspUrlChanged 발화
   → _tryAttach()
      → getWorkerBySlot(slotId=5)
      → m_slotToUrl[5] = "rtsp://B"  ✅ 이미 갱신됨
      → getWorker("rtsp://B") = workerB  ✅
      → root.worker = workerB  ✅ B 스트림 연결

④ emit dataChanged(index=1, {RtspUrlRole, ...})
   → VideoSurface_1.onRtspUrlChanged
   → _tryAttach()
      → getWorkerBySlot(slotId=7)
      → m_slotToUrl[7] = "rtsp://A"  ✅
      → getWorker("rtsp://A") = workerA  ✅
      → root.worker = workerA  ✅ A 스트림 연결

결과:
  위치 0 카드: slotId=5, rtsp://B → workerB → B 스트림 ✅
  위치 1 카드: slotId=7, rtsp://A → workerA → A 스트림 ✅
  예측 불가 타이밍 없음 ✅
  잘못된 worker 연결 없음 ✅
  오염 없음 ✅
```
