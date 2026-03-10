# Swap & Streaming 버그 분석 및 개선 명세

> 작성일: 2026-03-10  
> 현상: Swap 이 안 되거나, Swap 후 스트리밍이 안 됨, 예측 불가능한 타이밍에 스트리밍이 됨

---

## 전체 원인 지도

```
증상 1: Swap 이 아예 안 됨
  └─ [BUG-1] ghost.Drag.drop() 실행 순서가 잘못됨
             dragSourceIndex = -1 이 drop() 보다 먼저 실행되어
             ghost.dragIndex = -1 이 되어 src = -1 → swap 조건 실패

증상 2: Swap 은 됐는데 스트리밍이 안 됨
  └─ [BUG-2] swapSlots 후 _emitSlotsUpdated() 미호출
             m_slotToUrl 이 갱신되지 않아 getWorkerBySlot(slotId)
             가 swap 전 URL 의 worker 를 계속 반환함

  └─ [BUG-3] VideoSurface 가 rtspUrl 변경을 감지하지 못함
             slotId 는 안 바뀌고 rtspUrl 만 바뀌는데,
             onRtspUrlChanged 는 slotId < 0 일 때만 동작함

증상 3: 예측 불가능한 타이밍에 스트리밍이 됨
  └─ [BUG-4] reuseItems: true + slotId 가 다른 delegate 로 재사용
             GridView 가 delegate 를 재활용할 때 slotId 가 바뀌면
             VideoSurface.onSlotIdChanged 가 발화해서 우연히 연결됨
             → 완전히 다른 카드의 스트림이 잠깐 보이는 현상

  └─ [BUG-5] VideoSurfaceItem.updatePaintNode 에서 매 프레임
             createTextureFromImage 를 호출하여 기존 캐시가 무시됨
             (텍스처 재사용 로직의 조건 버그)
```

---

## BUG-1: `ghost.Drag.drop()` 실행 순서 오류 — Swap 자체가 안 됨

### 위치
`Qt/Front/View/DashboardPage.qml` — `onDragEndedOutside` 핸들러

### 현재 코드
```qml
onDragEndedOutside: (sourceIndex, slotId, ...) => {
    ghost.Drag.drop();           // (1) drop 시도
    root.dragSourceIndex = -1;  // (2) dragSourceIndex 초기화
    ...
}
```

### ghost 의 dragIndex 바인딩
```qml
Rectangle {
    id: ghost
    property int dragIndex: root.dragSourceIndex  // dragSourceIndex 에 실시간 바인딩
    Drag.active: root.dragSourceIndex >= 0
    ...
}
```

### 문제 분석

`ghost.Drag.drop()` 은 현재 `ghost` 가 겹쳐 있는 `DropArea.onDropped` 를 동기적으로 호출한다.  
`DropArea.onDropped` 내부에서 `drag.source.dragIndex` 를 읽는다:

```qml
onDropped: drag => {
    const src = drag.source.dragIndex;  // ghost.dragIndex 를 읽음
    ...
    if (src !== dst && src >= 0)
        root.slotSwapped(src, dst);     // src = -1 이면 이 줄이 실행 안 됨
}
```

**실제 실행 흐름:**

`DragHandler.onActiveChanged` (active = false) 가 발화하는 시점에  
`root.dragSourceIndex` 는 아직 유효한 값(예: 2)이다.  
그런데 `onDragEndedOutside` 시그널은 Layout 에서 emit 되고,  
DashboardPage 에서 해당 핸들러가 실행되는 것은 **QML 이벤트 루프의 다음 사이클**일 수도 있다.

더 근본적인 문제는 `ghost.Drag.active` 의 바인딩이다:

```qml
Drag.active: root.dragSourceIndex >= 0
```

`ghost.Drag.drop()` 이 호출되는 시점에 `root.dragSourceIndex` 가 이미 `-1` 이 됐다면  
`Drag.active = false` 상태이므로 `drop()` 호출 자체가 **무시**된다.

실제로는 `onDragEndedOutside` 핸들러 내에서 `ghost.Drag.drop()` 과  
`root.dragSourceIndex = -1` 의 순서가 보장되는 것처럼 보이지만,  
QML 바인딩 엔진은 프로퍼티 변경을 **즉시 전파**하므로  
`root.dragSourceIndex = -1` 직전에 `ghost.Drag.active` 가 `false` 로 바뀔 수 있다.

### 정확한 실패 시나리오

```
DragHandler.onActiveChanged(active=false) 발화
  → emit dragEndedOutside(sourceIndex=2, slotId=5, ...)

[QML 이벤트 큐]
  DashboardPage.onDragEndedOutside 실행:
    ghost.Drag.drop()        ← ghost.Drag.active = (dragSourceIndex >= 0) = (2 >= 0) = true
                             ← DropArea.onDropped 발화 → drag.source.dragIndex = 2 ✅
    root.dragSourceIndex = -1
```

위 경우는 작동한다. 그런데 다음 경우:

```
DragHandler.onActiveChanged(active=false) 발화와 동시에
다른 QML 이벤트(예: mousemove)가 먼저 처리돼서
dragSourceIndex 가 먼저 -1 이 되면:

  ghost.Drag.drop() 호출 시 ghost.Drag.active = false → 무시됨
  → DropArea.onDropped 미발화 → slotSwapped 미발화 → Swap 안 됨
```

### 해결책

`ghost.Drag.active` 를 `root.dragSourceIndex` 바인딩에서 분리하고,  
명시적으로 제어한다. `drop()` 완료 후 `dragSourceIndex` 를 초기화한다.

```qml
// DashboardPage.qml — ghost 수정
Rectangle {
    id: ghost
    property int dragIndex: root.dragSourceIndex

    // ← 바인딩 제거, 수동 제어로 전환
    // Drag.active: root.dragSourceIndex >= 0   ← 이 줄 삭제

    Drag.active: false   // 초기값, JS 에서 명시적으로 제어
    Drag.source: ghost
    Drag.keys: ["cameraCard"]
    ...
}

// onDragStarted: Drag.active 활성화
onDragStarted: (index, globalX, globalY) => {
    root.dragSourceIndex = index;
    ghost.Drag.active = true;          // ← 명시적 활성화
    ...
}

// onDragEndedOutside: drop() → active 비활성화 → dragSourceIndex 초기화 순서 보장
onDragEndedOutside: (sourceIndex, slotId, title, url, isOnline, cropRect, globalX, globalY) => {
    // 1. drop() 호출 — Drag.active = true 인 상태에서 반드시 실행
    ghost.Drag.drop();

    // 2. Drag 시스템 종료
    ghost.Drag.active = false;

    // 3. 상태 초기화
    root.dragSourceIndex = -1;

    // 4. 외부 드래그 → 새 윈도우 생성 판정
    const pos = root.mapFromItem(gridLayout, globalX, globalY);
    const swPos = root.mapFromItem(swapWindowArea, 0, 0);
    const isOutside = pos.x < swPos.x || pos.x > (swPos.x + swapWindowArea.width)
                   || pos.y < swPos.y || pos.y > (swPos.y + swapWindowArea.height);
    if (isOutside) {
        root.tryDetachWindow(sourceIndex, slotId, title, url, isOnline, cropRect, pos.x, pos.y);
    }
}
```

---

## BUG-2: `swapSlots` 후 `_emitSlotsUpdated()` 미호출 — VideoManager 맵 미갱신

### 위치
`Qt/Back/CameraModel.cpp` — `swapSlots()`

### 현재 코드
```cpp
void CameraModel::swapSlots(int indexA, int indexB) {
    ...
    std::swap(a.rtspUrl, b.rtspUrl);
    std::swap(a.title,   b.title);
    std::swap(a.cameraType, b.cameraType);
    std::swap(a.isOnline,   b.isOnline);
    std::swap(a.description, b.description);

    emit dataChanged(idxA, idxA, roles);
    emit dataChanged(idxB, idxB, roles);
    // ← _emitSlotsUpdated() 호출 없음!
}
```

### 문제 분석

`swapSlots` 는 rtspUrl 을 교환하지만 `_emitSlotsUpdated()` 를 호출하지 않는다.  
이 함수의 역할은 `emit slotsUpdated(...)` → `VideoManager::registerSlots()` 호출이다.

`VideoManager::registerSlots()` 는:
```cpp
void VideoManager::registerSlots(const QList<SlotInfo> &Slots) {
    m_slotToUrl.clear();                    // slotId → rtspUrl 맵 전체 재구성
    for (const SlotInfo &si : Slots)
        m_slotToUrl.insert(si.slotId, si.rtspUrl);
    ...
}
```

**Swap 후 m_slotToUrl 상태:**

```
Swap 전:
  m_cameras[0]: slotId=5, rtspUrl="rtsp://A"
  m_cameras[1]: slotId=7, rtspUrl="rtsp://B"
  m_slotToUrl:  { 5 → "rtsp://A", 7 → "rtsp://B" }

swapSlots(0, 1) 실행 후:
  m_cameras[0]: slotId=5, rtspUrl="rtsp://B"   ← rtspUrl 교환됨
  m_cameras[1]: slotId=7, rtspUrl="rtsp://A"

  m_slotToUrl:  { 5 → "rtsp://A", 7 → "rtsp://B" }  ← 갱신 안 됨!
```

`VideoSurface` 가 `videoManager.getWorkerBySlot(5)` 를 호출하면:
```
m_slotToUrl[5] = "rtsp://A"  (swap 전 URL)
→ m_workers["rtsp://A"] 반환
→ 여전히 A 스트림을 보여줌
```

Swap 이 모델에서는 됐지만 스트림은 교환이 안 되는 이유가 이것이다.

### 해결책

`swapSlots` 마지막에 `_emitSlotsUpdated()` 를 추가한다.

```cpp
void CameraModel::swapSlots(int indexA, int indexB) {
    if (indexA < 0 || indexB < 0 || indexA >= m_cameras.size() ||
        indexB >= m_cameras.size() || indexA == indexB)
        return;

    auto &a = m_cameras[indexA];
    auto &b = m_cameras[indexB];
    std::swap(a.rtspUrl,      b.rtspUrl);
    std::swap(a.title,        b.title);
    std::swap(a.cameraType,   b.cameraType);
    std::swap(a.isOnline,     b.isOnline);
    std::swap(a.description,  b.description);

    const auto idxA = createIndex(indexA, 0);
    const auto idxB = createIndex(indexB, 0);
    const QList<int> roles = {TitleRole, RtspUrlRole, IsOnlineRole,
                               DescriptionRole, CameraTypeRole};
    emit dataChanged(idxA, idxA, roles);
    emit dataChanged(idxB, idxB, roles);

    _emitSlotsUpdated();   // ← 이 줄 추가: VideoManager::m_slotToUrl 갱신 트리거
}
```

---

## BUG-3: `VideoSurface` 가 Swap 후 rtspUrl 변경을 감지하지 못함

### 위치
`Qt/Front/Component/VideoSurface.qml`

### 현재 코드
```qml
onRtspUrlChanged: {
    if (root.slotId < 0) {       // ← slotId 가 유효하면 무시!
        ...
        _tryAttach();
    }
}
```

### 문제 분석

Swap 후 `dataChanged(RtspUrlRole)` 이 발화되어  
`CameraCard.rtspUrl` → `VideoSurface.rtspUrl` 이 새 URL 로 바뀐다.

그런데 `VideoSurface.onRtspUrlChanged` 는 `slotId < 0` 일 때만 `_tryAttach()` 를 호출한다.  
slotId 가 유효한 경우(= 정상 경우) 에는 아무 일도 일어나지 않는다.

`_tryAttach()` 의 현재 slotId 기반 조회:
```qml
function _tryAttach() {
    if (root.slotId >= 0) {
        w = videoManager.getWorkerBySlot(root.slotId);
        ...
    }
}
```

BUG-2 가 수정되어 `m_slotToUrl` 이 갱신되더라도,  
`_tryAttach()` 가 호출되지 않으면 새 URL 의 worker 로 전환되지 않는다.

**타이밍 경쟁 분석:**

```
swapSlots() 호출
  (1) dataChanged(RtspUrlRole) emit           → VideoSurface.rtspUrl 바뀜
                                               → onRtspUrlChanged 발화
                                               → slotId >= 0 이므로 무시 ← 문제
  (2) _emitSlotsUpdated() [BUG-2 수정 후]
      → slotsUpdated emit
      → VideoManager::registerSlots()
      → m_slotToUrl 갱신
      → 신규 URL 없음 → workerRegistered emit 없음
      → VideoSurface._onRegistered 미발화
```

결과: VideoSurface 의 worker 는 swap 전 worker 를 유지한다.

### 해결책

`onRtspUrlChanged` 에서 `slotId >= 0` 인 경우에도 `_tryAttach()` 를 호출한다.  
단, 불필요한 worker 탐색을 막기 위해 현재 worker 의 URL 과 비교하여  
변경이 있을 때만 재연결한다.

```qml
// VideoSurface.qml 수정

onRtspUrlChanged: {
    // slotId 유무와 관계없이 항상 재연결 시도
    // (slotId 기반이든 URL 기반이든 _tryAttach 내부에서 처리)
    try {
        videoManager.workerRegistered.disconnect(_onRegistered);
    } catch (e) {}
    _tryAttach();
}
```

---

## BUG-4: `reuseItems: true` + `slotId` 변경으로 인한 예측 불가 스트리밍

### 위치
`Qt/Front/Layout/DashboardLayout.qml` — GridView

### 현재 코드
```qml
GridView {
    ...
    reuseItems: true   // ← 문제의 원인
    ...
}
```

### 문제 분석

`reuseItems: true` 는 GridView 가 화면 밖으로 스크롤된 delegate 를  
**파괴하지 않고 재활용**한다. 재활용된 delegate 는 새 model 데이터로 바인딩이 갱신된다.

Swap 발생 시:
1. `swapSlots(0, 1)` → `dataChanged` emit
2. GridView 가 index 0, 1 의 delegate 를 업데이트
3. delegate 의 `model.slotId` 가 바뀜 (A→B, B→A)
4. `CameraCard.slotId` 바인딩 업데이트 → `VideoSurface.slotId` 변경
5. `VideoSurface.onSlotIdChanged` 발화 → `_tryAttach()` 호출

**현재 Swap 설계 (slotId 는 유지, rtspUrl 만 교환)에서:**

Swap 후 slotId 는 변하지 않으므로 `onSlotIdChanged` 가 발화하지 않아야 한다.  
그런데 `reuseItems: true` 일 때 GridView 는 **delegate 를 다른 index 의 데이터로 교체**할 수 있다.  
예를 들어 스크롤 후 index 0 의 delegate 가 index 5 의 데이터로 재사용되면  
`model.slotId` 가 완전히 다른 값으로 바뀌어 `onSlotIdChanged` 가 발화한다.

이때 `_tryAttach()` 가 호출되어 새 slotId 에 해당하는 worker 를 연결하면,  
이전 슬롯의 스트림이 잠깐 보이다가 새 슬롯 스트림으로 전환되는 **플리커** 현상이 발생한다.

또한 `reuseItems: true` 상태에서 `Component.onCompleted` 는 **첫 생성 시 1회만** 호출된다.  
재사용 시에는 `Component.onCompleted` 가 호출되지 않는다.  
`VideoSurface.Component.onCompleted: _tryAttach()` 가 재사용 시 실행되지 않는다는 의미이다.

반면 QML Item `Accessible.onActiveFocusItemChanged` 등 일부 바인딩은 재사용 시  
예상치 못한 시점에 발화하여 worker 가 null 로 리셋될 수 있다.

### 해결책

**Option A (권장): `reuseItems: false` 로 변경**

```qml
GridView {
    ...
    reuseItems: false   // delegate 를 항상 새로 생성/파괴
    ...
}
```

13개 카메라 정도의 규모에서는 delegate 생성/파괴 비용이 무시할 수 있는 수준이다.  
`reuseItems: true` 의 이점(스크롤 성능)은 수백 개 이상의 아이템에서 유효하다.

**Option B: `Loader` 를 VideoSurface 의 재사용 경계로 사용 (CameraCard 에서 이미 사용 중)**

이미 `CameraCard.qml` 내부에 `Loader { active: root.isOnline && root.rtspUrl !== "" }` 가 있다.  
`isOnline` 또는 `rtspUrl` 이 바뀌면 Loader 가 VideoSurface 를 파괴/재생성하므로  
자연스러운 재연결이 된다. `reuseItems: true` 를 유지하려면 이 경로를 신뢰해야 하지만,  
BUG-3 문제로 인해 rtspUrl 변경이 VideoSurface 에서 무시되므로 Option A 가 더 안전하다.

---

## BUG-5: `VideoSurfaceItem.updatePaintNode` 에서 텍스처 캐시가 실제로 작동하지 않음

### 위치
`Qt/Back/VideoSurfaceItem.cpp` — `updatePaintNode()`

### 현재 코드 분석

```cpp
// 캐시 확인 로직이 없다
QSGTexture *newTex =
    window()->createTextureFromImage(img, QQuickWindow::TextureIsOpaque);

if (newTex) {
    node->setTexture(newTex); // 매 프레임 새 텍스처 할당
    m_cachedTex = newTex;
    m_cachedW = frame.width;
    m_cachedH = frame.height;
}
```

`m_cachedTex`, `m_cachedW`, `m_cachedH` 는 선언되어 있지만  
**실제로 캐시를 활용하는 조건 분기가 없다.**  
`createTextureFromImage` 는 매 프레임 호출되어 매 프레임 GPU 텍스처를 새로 할당한다.

이것은 헤더 주석(`§2: A new QSGTexture is only allocated when frame dimensions change`)과  
**완전히 다른 동작**이다.

이로 인해 13 카메라 × 24fps = **312회/초 GPU 텍스처 할당**이 발생한다.  
GPU 드라이버의 텍스처 할당/해제 처리 시간이 불규칙하여  
스트리밍 렌더링에 예측 불가능한 지연이 발생하는 이유 중 하나이다.

### 올바른 텍스처 재사용 구현

해상도가 바뀔 때만 새 텍스처를 생성하고, 동일 해상도에서는 기존 텍스처에 픽셀 데이터만 업데이트한다.

그런데 `QSGTexture` API 에는 `setImage()` 또는 픽셀 데이터 업데이트 메서드가 없다.  
`createTextureFromImage` 는 항상 새 텍스처를 반환한다.

**실제 해결책**: `QSGTexture` 레벨에서 재사용은 Qt 공식 API 로는 불가능하다.  
대신 `QSGDynamicTexture` 를 서브클래싱하거나 `QRhi` 를 직접 사용해야 한다.  
그러나 이것은 복잡한 변경이므로, **현재 구조를 유지하면서 비용을 줄이는 방법**을 택한다.

**현실적 개선**: `onFrameReady` 에서 프레임을 GPU로 올리는 시점을 제어하여  
중복 업로드를 줄인다. 현재 `m_updatePending` 플래그로 coalescing 은 이미 되어 있다.  
추가로 실제 데이터가 변경된 프레임만 렌더링 스케줄에 올린다.

```cpp
// VideoSurfaceItem.cpp — updatePaintNode 개선
// 핵심: 같은 buffer 포인터면 텍스처 재생성 안 함

QSGNode *VideoSurfaceItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    m_updatePending.store(false, std::memory_order_release);

    if (!m_worker) {
        delete oldNode;
        m_cachedTex = nullptr;
        m_cachedW = 0;
        m_cachedH = 0;
        m_renderBufferHold.reset();
        return nullptr;
    }

    VideoWorker::FrameData frame = m_worker->getLatestFrame();

    if (!frame.buffer || frame.width <= 0 || frame.height <= 0) {
        m_renderBufferHold.reset();
        return oldNode;
    }
    if (frame.stride <= 0)
        frame.stride = frame.width * 4;

    QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);
    if (!node) {
        node = window()->createImageNode();
        node->setFiltering(QSGTexture::Linear);
        node->setOwnsTexture(true);
        m_cachedTex = nullptr;
        m_cachedW   = 0;
        m_cachedH   = 0;
    }

    // ── 핵심 캐시 조건 ──────────────────────────────────────────────────────
    // 같은 버퍼 포인터 + 같은 해상도 → 텍스처 재생성 불필요
    const bool sameBuffer = (frame.buffer == m_renderBufferHold);
    const bool sameDims   = (frame.width == m_cachedW && frame.height == m_cachedH);

    if (!sameBuffer || !sameDims) {
        // 새 프레임 또는 해상도 변경 시에만 텍스처 갱신
        m_renderBufferHold = frame.buffer;

        const QImage img(m_renderBufferHold->data(),
                         frame.width, frame.height,
                         frame.stride, QImage::Format_RGBA8888);

        QSGTexture *newTex = window()->createTextureFromImage(
            img, QQuickWindow::TextureIsOpaque);

        if (newTex) {
            node->setTexture(newTex);
            m_cachedTex = newTex;
            m_cachedW   = frame.width;
            m_cachedH   = frame.height;
        }
    }

    node->setRect(boundingRect());
    const qreal nx = std::clamp(m_cropRect.x(),     0.0, 1.0);
    const qreal ny = std::clamp(m_cropRect.y(),     0.0, 1.0);
    qreal nw = std::clamp(m_cropRect.width(),  0.0, 1.0 - nx);
    qreal nh = std::clamp(m_cropRect.height(), 0.0, 1.0 - ny);
    if (nw <= 0.0 || nh <= 0.0) { nw = 1.0; nh = 1.0; }
    node->setSourceRect(QRectF(nx * frame.width,  ny * frame.height,
                               nw * frame.width,  nh * frame.height));
    return node;
}
```

---

## 전체 수정 요약

| # | 파일 | 변경 | 해결하는 증상 |
|---|------|------|-------------|
| **1** | `Qt/Front/View/DashboardPage.qml` | `ghost.Drag.active` 를 수동 제어로 변경. `drop()` → `active=false` → `dragSourceIndex=-1` 순서 명시 | Swap 이 아예 안 됨 |
| **2** | `Qt/Back/CameraModel.cpp` | `swapSlots()` 끝에 `_emitSlotsUpdated()` 추가 | Swap 후 VideoManager m_slotToUrl 미갱신 |
| **3** | `Qt/Front/Component/VideoSurface.qml` | `onRtspUrlChanged` 에서 `slotId >= 0` 조건 제거, 항상 `_tryAttach()` 호출 | Swap 후 스트리밍 전환 안 됨 |
| **4** | `Qt/Front/Layout/DashboardLayout.qml` | `GridView.reuseItems: false` 로 변경 | 예측 불가능한 스트리밍 타이밍 |
| **5** | `Qt/Back/VideoSurfaceItem.cpp` | `updatePaintNode` 에 버퍼 포인터 캐시 조건 추가 | 매 프레임 GPU 텍스처 재할당 |

---

## 수정 후 Swap 정상 동작 시퀀스

```
사용자: 카드 A (slotId=5, rtsp://A) 를 카드 B (slotId=7, rtsp://B) 위로 드래그

1. DragHandler.onActiveChanged(active=true)
   → emit dragStarted(index=0, ...)
   → DashboardPage: dragSourceIndex=0, ghost.Drag.active=true [BUG-1 수정]

2. 사용자 드롭
   DragHandler.onActiveChanged(active=false)
   → emit dragEndedOutside(0, ...)
   → DashboardPage.onDragEndedOutside:
       ghost.Drag.drop()                    [ghost.Drag.active=true 보장]
         → DropArea(index=1).onDropped
         → drag.source.dragIndex = 0        [BUG-1 수정: -1 이 아님]
         → emit slotSwapped(0, 1)
         → cameraModel.swapSlots(0, 1)

3. CameraModel.swapSlots(0, 1):
   swap(rtspUrl): [0]="rtsp://B", [1]="rtsp://A"
   emit dataChanged(index 0, RtspUrlRole)   → CameraCard[0].rtspUrl="rtsp://B"
   emit dataChanged(index 1, RtspUrlRole)   → CameraCard[1].rtspUrl="rtsp://A"
   _emitSlotsUpdated()                      [BUG-2 수정]
     → emit slotsUpdated([{5,"rtsp://B"},{7,"rtsp://A"}])
     → VideoManager::registerSlots()
     → m_slotToUrl = { 5→"rtsp://B", 7→"rtsp://A" }

4. VideoSurface[slotId=5].onRtspUrlChanged("rtsp://B"):
   [BUG-3 수정: slotId 조건 제거]
   → _tryAttach()
   → getWorkerBySlot(5) → m_slotToUrl[5]="rtsp://B" → workers["rtsp://B"]
   → root.worker = B 워커
   → frameReady 연결 → B 스트림 렌더링 시작

5. VideoSurface[slotId=7].onRtspUrlChanged("rtsp://A"):
   → _tryAttach() → getWorkerBySlot(7) → workers["rtsp://A"]
   → root.worker = A 워커
   → A 스트림 렌더링 시작

결과: 카드 A 위치에 B 스트림, 카드 B 위치에 A 스트림 → 정상 Swap ✅
```
