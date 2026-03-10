# AnoMap — Bug & Feature Specification

> **Purpose:** Precise When / Where / Why / How documentation for every
> reported bug and feature request in this session.  
> **Audience:** Any developer picking up a ticket from this list.  
> **Format:** Each item is self-contained. Read only the section you need.

---

## Index

| # | Title | Type | Priority |
|---|-------|------|----------|
| [B-1](#b-1-mypagewhere-unable-to-assign-undefined-to-qcolor) | `MyPage.qml` — `Unable to assign [undefined] to QColor` | Bug | 🔴 Crash-class |
| [B-2](#b-2-devicelistlayoutqml-unable-to-assign-undefined-to-qrectf--int) | `DeviceListLayout.qml` — `Unable to assign [undefined] to QRectF / int` | Bug | 🔴 Crash-class |
| [B-3](#b-3-camera-card-not-visible-in-device--ai-pages) | Camera Card not visible in Device / AI pages | Bug | 🔴 Rendering |
| [B-4](#b-4-camera-card-swap-does-not-work) | Camera Card Swap does not work | Bug | 🟡 Feature |
| [F-1](#f-1-device-control--motor-wasd-forces-auto-off--sends-unauto) | Device Control — WASD forces Auto OFF + sends `unauto` | Feature | 🟡 Behaviour |
| [F-2](#f-2-camera-reconnect-on-online--offline--online-transition) | Camera reconnect on Online → Offline → Online transition | Feature | 🟡 Reliability |

---

## B-1 `MyPage.qml` — `Unable to assign [undefined] to QColor`

### Error output (exact)
```
qrc:/qt/qml/AnoMap/front/View/MyPage.qml:28:17: Unable to assign [undefined] to QColor
qrc:/qt/qml/AnoMap/front/View/MyPage.qml:33:21: Unable to assign [undefined] to QColor
qrc:/qt/qml/AnoMap/front/View/MyPage.qml:53:17: Unable to assign [undefined] to QColor
qrc:/qt/qml/AnoMap/front/View/MyPage.qml:63:21: Unable to assign [undefined] to QColor
qrc:/qt/qml/AnoMap/front/View/MyPage.qml:69:21: Unable to assign [undefined] to QColor
qrc:/qt/qml/AnoMap/front/View/MyPage.qml:78:25: Unable to assign [undefined] to QColor
```

### When
At QML component creation time — the instant `MyPage.qml` is first loaded into
the QML engine, before the user navigates to it.

### Where

**File:** `Qt/Front/View/MyPage.qml`  
**File:** `Qt/Front/Theme/Theme.qml`

**Affected lines in `MyPage.qml`:**
```qml
// line 28 — color: Theme.colorSecondary        ← does NOT exist in Theme.qml
// line 33 — color: Theme.colorBackground       ← does NOT exist in Theme.qml
// line 53 — color: Theme.colorSecondary        ← does NOT exist in Theme.qml
// line 63 — color: Theme.colorPrimary          ← does NOT exist in Theme.qml
// line 63 — color: Theme.colorSecondary        ← does NOT exist in Theme.qml
// line 69 — color: Theme.colorBackground       ← does NOT exist in Theme.qml
// line 78 — color: Theme.fontColorSecondary    ← does NOT exist in Theme.qml
```

**Actual properties defined in `Theme.qml`:**
```qml
// Theme.qml — complete property list
property color hanwhaFirst:   "#F37321"
property color hanwhaSecond:  "#F89B6C"
property color hanwhaThird:   "#FBB584"
property color bgPrimary:     ...
property color bgSecondary:   ...
property color bgThird:       ...
property color bgAuto:        ...
property color fontColor:     ...
property color iconColor:     ...
property color iconChange:    ...
```

### Why
`MyPage.qml` uses a **different naming convention** (`colorSecondary`,
`colorBackground`, `colorPrimary`, `fontColorSecondary`) than the one
`Theme.qml` actually exports (`bgSecondary`, `bgPrimary`, `fontColor`, etc.).
The page was authored against a planned Theme API that was never implemented,
or was renamed during a refactor without updating `MyPage.qml`.

QML resolves `Theme.colorSecondary` as `undefined` at binding time. Assigning
`undefined` to a `color` property is a hard type error → the engine logs the
warning and leaves the property at its default value (usually transparent /
black). The UI renders but looks broken.

### How — Fix mapping table

Replace every mismatched property name in `MyPage.qml`:

| `MyPage.qml` uses (wrong) | Replace with (correct Theme.qml name) | Semantic |
|--------------------------|--------------------------------------|---------|
| `Theme.colorSecondary`   | `Theme.bgSecondary`                  | Secondary background surface |
| `Theme.colorBackground`  | `Theme.bgPrimary`                    | Main background |
| `Theme.colorPrimary`     | `Theme.hanwhaFirst`                  | Brand/accent colour |
| `Theme.fontColorSecondary` | `Theme.fontColor`                  | Body text (no secondary variant exists yet) |
| `Theme.fontColor`        | `Theme.fontColor`                    | ✅ Already correct |

**Option A — Fix only `MyPage.qml` (minimal change, recommended):**  
Do a search-and-replace within `MyPage.qml`:

```
Theme.colorSecondary   → Theme.bgSecondary
Theme.colorBackground  → Theme.bgPrimary
Theme.colorPrimary     → Theme.hanwhaFirst
Theme.fontColorSecondary → Theme.fontColor
```

**Option B — Extend `Theme.qml` with alias properties:**  
Add read-only alias properties to `Theme.qml` so both naming conventions work.
Only choose this if other pages also use the old names.

```qml
// Add to Theme.qml:
readonly property color colorPrimary:       hanwhaFirst
readonly property color colorSecondary:     bgSecondary
readonly property color colorBackground:    bgPrimary
readonly property color fontColorSecondary: fontColor
```

**Recommendation:** Use Option A. The old names are confusing (`colorBackground`
vs `bgPrimary`). Keep one consistent convention.

---

## B-2 `DeviceListLayout.qml` — `Unable to assign [undefined] to QRectF / int`

### Error output (exact)
```
qrc:/qt/qml/AnoMap/front/Layout/DeviceListLayout.qml:45:21: Unable to assign [undefined] to QRectF
qrc:/qt/qml/AnoMap/front/Layout/DeviceListLayout.qml:44:21: Unable to assign [undefined] to int
[DeviceModel] onStoreUpdated, count: 1
```

### When
When `DevicePage.qml` becomes visible and `DeviceListLayout.qml` instantiates
its delegate for the one SUB_PI device that `DeviceModel` has just reported.

### Where

**File:** `Qt/Front/Layout/DeviceListLayout.qml` — lines 44–45 (inside the
`ListView` delegate, inside `CameraCard`):

```qml
// DeviceListLayout.qml, inside delegate → CameraCard:
CameraCard {
    ...
    slotId:   model.slotId    // line 44 — model is DeviceModel, not CameraModel
    cropRect: model.cropRect  // line 45 — model is DeviceModel, not CameraModel
    ...
}
```

**Root cause:** `DeviceListLayout.qml` drives its `ListView` from `deviceModel`
(`DeviceModel`). `DeviceEntry` / `DeviceModel` roles are:
```
RtspUrlRole, DeviceIpRole, TitleRole, IsOnlineRole,
HasMotorRole, HasIrRole, HasHeaterRole
```
There is **no** `slotId` role and **no** `cropRect` role in `DeviceModel`.
When QML reads `model.slotId` and `model.cropRect` from a `DeviceModel`
delegate, both return `undefined`.

`CameraCard` declares:
```qml
property int slotId: -1       // expects int
property rect cropRect: ...   // expects QRectF
```
Assigning `undefined` → type error at binding time.

### Why
`DeviceListLayout.qml` was updated to pass the new `slotId` and `cropRect`
properties (added in the VIDEO_STREAMING_SPEC refactor) to `CameraCard`, but
the backing model is `DeviceModel`, which was never extended with those roles.
`CameraModel` has them; `DeviceModel` does not and does not need them for its
own purpose (device control, not video tiling).

### How

**Two sub-fixes required:**

#### Fix A — `DeviceListLayout.qml`: supply safe fallback values

`DeviceModel` will never carry `slotId` or `cropRect` — that is correct. The
layout must resolve the video worker another way.

**Option 1 (recommended):** Look up the `slotId` from `CameraModel` by `rtspUrl`:

```qml
// In DeviceListLayout.qml delegate, replace:
slotId:   model.slotId
cropRect: model.cropRect

// With:
slotId:   typeof cameraModel !== "undefined"
              ? cameraModel.slotIdForUrl(model.rtspUrl)   // ← new invokable
              : -1
cropRect: Qt.rect(0, 0, 1, 1)   // device view always shows full frame
```

**Option 2 (simplest short-term):** Remove the two lines and use hard-coded
safe defaults — `DeviceListLayout` cards never need tile splitting:

```qml
CameraCard {
    // slotId and cropRect intentionally omitted →
    // CameraCard defaults: slotId=-1, cropRect=(0,0,1,1)
    // VideoSurface will fall back to getWorker(rtspUrl)
    title:    model.title
    rtspUrl:  model.rtspUrl
    isOnline: model.isOnline
    showActionIcon: false
    draggable:      false
    highlightOnHover: false
}
```

#### Fix B — `CameraModel`: add `Q_INVOKABLE int slotIdForUrl(const QString &rtspUrl) const`

Needed only if Option 1 is chosen. Iterates `m_cameras`, returns the `slotId`
of the first entry whose `rtspUrl` matches, or `-1` if not found.

```cpp
// CameraModel.hpp — add declaration:
Q_INVOKABLE int slotIdForUrl(const QString &rtspUrl) const;

// CameraModel.cpp — implementation:
int CameraModel::slotIdForUrl(const QString &rtspUrl) const {
    for (const auto &e : m_cameras)
        if (e.rtspUrl == rtspUrl) return e.slotId;
    return -1;
}
```

**Recommendation:** Use Fix A Option 2 now (remove the undefined lines).
Add `slotIdForUrl` as Fix B later if Device page cameras need tile-accurate
worker lookup.

---

## B-3 Camera Card not visible in Device / AI Pages

### When
When the user navigates to the **Device** tab or the **AI** tab after a
successful login. Camera cards are expected to appear; instead the page is
blank or shows the "No cameras / No devices" placeholder.

### Where

| Page | File | Model used | Expected card |
|------|------|-----------|---------------|
| Device | `Qt/Front/View/DevicePage.qml` | `deviceModel` (DeviceModel) | SUB_PI card with DeviceControlBar |
| AI | `Qt/Front/View/AIPage.qml` | `cameraModel` (CameraModel) | All HANWHA cards |

### Why

**Device page — probable cause:**  
The `DeviceListLayout` is conditionally visible:
```qml
// DevicePage.qml
DeviceListLayout {
    visible: typeof deviceModel !== "undefined" && deviceModel.count > 0
```
`DeviceModel` exposes `rowCount()` via `QAbstractListModel`. In QML, the count
is accessed as `.count` (QAbstractListModel maps `rowCount()` → `count`
automatically). However, if `DeviceModel` is registered as a context property
but `onStoreUpdated` has not yet been called (no CAMERA packet received yet),
`count` is `0` → the layout stays hidden.

The crash-class bug in B-2 (`Unable to assign [undefined]`) is a **secondary
blocker**: even after `deviceModel.count > 0` becomes true and the layout
becomes visible, the delegate immediately throws a binding error and the cards
may render blank or not render at all because `slotId` and `cropRect` are
undefined.

**AI page — probable cause:**  
`AIPage.qml` uses `cameraModel` which is populated from the CAMERA (0x07)
packet. The model is populated correctly (`CameraModel::onStoreUpdated` is
confirmed working). The AI page grid is conditionally visible:
```qml
visible: typeof cameraModel !== "undefined" && cameraModel.count > 0
```
If the AI page is opened before the first CAMERA packet arrives, `count` is
`0` → blank. After the first packet the grid should appear. This is likely a
**timing** issue, not a structural one.

However, `AIPage.qml` passes `slotId: model.slotId` and `cropRect: model.cropRect`
to `CameraCard`. These DO exist in `CameraModel` so the bindings are valid.
If cards still do not appear, verify that `cameraModel.count > 0` is true
when the page is active by adding a temporary `Text { text: cameraModel.count }`.

**Summary of root causes:**

| Page | Root cause | Fix |
|------|-----------|-----|
| Device | B-2 binding error breaks delegate; model count may also be 0 on first open | Fix B-2 first; add count debug check |
| AI | Timing — page opened before CAMERA packet; possibly also `cameraType` filter needed | None needed if cards appear after packet; add filter if only HANWHA should show |

### How

**Device page:**
1. Apply fix from B-2 (remove `slotId`/`cropRect` from `DeviceListLayout`
   delegate or supply safe fallbacks).
2. After fix, open Device page. Confirm `[DeviceModel] onStoreUpdated, count: 1`
   appears in the log first. Then navigate to Device tab. Cards should appear.

**AI page filter (optional):**  
If the requirement is to show ONLY HANWHA cameras in the AI tab (not SUB_PI),
add a filter in the delegate:
```qml
// AIPage.qml delegate:
visible: model.cameraType === "HANWHA"
height: model.cameraType === "HANWHA" ? aiGrid.cellHeight : 0
```
Or use a `QSortFilterProxyModel` in C++ to filter by `cameraType === "HANWHA"`.

---

## B-4 Camera Card Swap Does Not Work

### When
When the user drags a `CameraCard` in `DashboardPage` and drops it onto
another card. The expectation is that the two cards exchange their visual
positions. Currently, nothing happens or the drag completes without swapping.

### Where

**Signal chain (what should happen):**
```
DragHandler (in DashboardLayout.qml)
  → onActiveChanged (drag released)
  → emit dragEndedOutside(...)   OR
  → DropArea::onDropped
       → emit slotSwapped(src, dst)   ← signals DashboardPage

DashboardPage.qml
  → onSlotSwapped: (src, dst) => cameraModel.swapSlots(src, dst)

CameraModel::swapSlots(indexA, indexB)
  → std::swap(m_cameras[indexA], m_cameras[indexB])
  → emit dataChanged(...)
```

**Current code in `DashboardLayout.qml` — `DragHandler::onActiveChanged`:**
```qml
DragHandler {
    id: dragHandler
    target: null
    onActiveChanged: {
        if (active) {
            // drag start — OK
            root.dragStarted(...)
        } else {
            // drag end — emits dragEndedOutside, NOT slotSwapped
            root.dragEndedOutside(...)
        }
    }
}
```

The `DragHandler.onActiveChanged` only emits `dragEndedOutside`. The
`slotSwapped` signal is emitted from the `DropArea.onDropped` handler.

**`DropArea.onDropped`:**
```qml
DropArea {
    id: cardDrop
    anchors.fill: parent
    keys: ["cameraCard"]

    onDropped: drag => {
        const src = drag.source.dragIndex;
        ...
        root.slotSwapped(src, dst);
    }
}
```

The `DropArea` listens for `keys: ["cameraCard"]`. The `Ghost` rectangle in
`DashboardPage.qml` has:
```qml
Rectangle {
    id: ghost
    Drag.active: root.dragSourceIndex >= 0
    Drag.source: ghost
    Drag.keys: ["cameraCard"]
    ...
}
```

### Why the swap fails

The `Ghost` is defined in `DashboardPage.qml`. The `DropArea` is inside
`DashboardLayout.qml`. The `Drag.drop()` call is **never explicitly made**
in the current `DragHandler.onActiveChanged`:

```qml
// DashboardLayout.qml — DragHandler.onActiveChanged (current code):
} else {
    // ← ghost.Drag.drop() is NEVER called here
    // The comment in the code says:
    // "Ghost를 Layout 외부에서 처리하므로 drop() 호출은 Page의 상태에 의존."
    root.dragEndedOutside(...);
}
```

Without calling `ghost.Drag.drop()`, Qt's drag-and-drop system never delivers
the `onDropped` event to any `DropArea`. The `DropArea.onDropped` → `slotSwapped`
path is therefore never triggered.

Additionally, `DashboardLayout.qml` cannot call `ghost.Drag.drop()` directly
because `ghost` is defined in `DashboardPage.qml`, not in the layout. The
layout has no reference to `ghost`.

### How — Fix

**Fix location:** `DashboardPage.qml` — in the `onDragEndedOutside` handler,
call `ghost.Drag.drop()` **before** the outside-boundary check:

```qml
// DashboardPage.qml
onDragEndedOutside: (sourceIndex, slotId, title, url, isOnline, cropRect, globalX, globalY) => {
    // 1. Attempt a drop — if the ghost is over a DropArea this will trigger
    //    DropArea.onDropped → DashboardLayout emits slotSwapped → swapSlots()
    const dropResult = ghost.Drag.drop();

    // 2. Only detach window if NOT a valid swap drop AND pointer is outside area
    if (dropResult !== Qt.MoveAction) {
        root.dragSourceIndex = -1;
        const pos = root.mapFromItem(gridLayout, globalX, globalY);
        const swPos = root.mapFromItem(swapWindowArea, 0, 0);
        const isOutside = pos.x < swPos.x || pos.x > (swPos.x + swapWindowArea.width)
                       || pos.y < swPos.y || pos.y > (swPos.y + swapWindowArea.height);
        if (isOutside) {
            root.tryDetachWindow(sourceIndex, slotId, title, url, isOnline, cropRect, pos.x, pos.y);
        }
    } else {
        root.dragSourceIndex = -1;
    }
}
```

**Why this works:**  
`ghost.Drag.drop()` delivers the Qt drop event to whatever `DropArea` the
ghost is currently overlapping. That `DropArea` is a card cell in `DashboardLayout`.
Its `onDropped` fires → `slotSwapped(src, dst)` is emitted →
`DashboardPage.onSlotSwapped` calls `cameraModel.swapSlots(src, dst)` →
`std::swap` runs → `dataChanged` emitted → QML grid re-renders with swapped cards.

**Verify `CameraModel::swapSlots` is connected:**  
In `DashboardPage.qml`:
```qml
onSlotSwapped: (sourceIndex, targetIndex) => {
    cameraModel.swapSlots(sourceIndex, targetIndex);   // ← must be present
}
```
Confirm this handler exists and that `swapSlots` matches `CameraModel`'s
`Q_INVOKABLE void swapSlots(int indexA, int indexB)`.

---

## F-1 Device Control — WASD Forces Auto OFF + Sends `unauto`

### When
When `DeviceControlBar` is open and the Motor Auto toggle is **ON** (`motorAuto
= true`), and the user presses any of the direction buttons: U (up / `w`),
D (down / `s`), L (left / `a`), R (right / `d`).

### Where
**File:** `Qt/Front/Component/device/DeviceControlBar.qml`

**Current code for direction buttons (unchanged from previous sessions):**
```qml
Btn {
    lbl: "U"
    onTapped: _motor("w")   // sends "w" packet only
}
```

**Current `_motor()` function:**
```qml
function _motor(v) {
    root.sendDeviceCmd(root.deviceIp, v, "", "");
    // ← does NOT touch motorAuto
    // ← does NOT send "unauto"
}
```

**Current Auto toggle:**
```qml
ToggleBtn {
    lbl: root.motorAuto ? "Motor Auto ON" : "Motor Auto OFF"
    active: root.motorAuto
    onTapped: {
        root.motorAuto = !root.motorAuto;
        _motor(root.motorAuto ? "auto" : "unauto");
        // ← only runs when toggle is explicitly clicked
    }
}
```

### Why
The server-side motor uses two modes:
- **Auto mode (`auto`):** the camera tracks objects autonomously. Manual
  direction commands are ignored or fight the auto algorithm.
- **Manual mode (`unauto`):** the user controls direction fully.

If the user presses a direction key while Auto is ON, the intent is clearly
"I want manual control right now". The UI must:
1. Immediately set `motorAuto = false` (visual feedback).
2. Send the direction packet (`w/a/s/d`) for the movement.
3. Send `"unauto"` to the server to disable auto-tracking on the device.

The current code only sends the direction packet — the device stays in auto
mode and may ignore or counteract the direction command.

The 1-second timer rule (from a previous session) specifies that `unauto`
should be sent once, **1 second after the last WASD press**, to batch rapid
key presses. This is the correct behaviour.

### How

**Step 1 — Add a `Timer` inside `DeviceControlBar.qml`:**
```qml
Timer {
    id: motorAutoTimer
    interval: 1000      // 1 second after the last WASD press
    repeat: false
    onTriggered: {
        // motorAuto was already set to false by _wasd(); send unauto once
        _motor("unauto");
    }
}
```

**Step 2 — Replace `_motor(direction)` calls in direction buttons with a new
`_wasd(direction)` function:**
```qml
function _wasd(dir) {
    _motor(dir);              // 1. send direction packet immediately
    root.motorAuto = false;   // 2. force Auto OFF in the UI
    motorAutoTimer.restart(); // 3. reset 1-second timer
    // unauto is sent by motorAutoTimer.onTriggered after 1 s of inactivity
}
```

**Step 3 — Update the four direction buttons to call `_wasd` instead of
`_motor`:**
```qml
Btn { lbl: "U"; onTapped: _wasd("w") }
Btn { lbl: "L"; onTapped: _wasd("a") }
Btn { lbl: "R"; onTapped: _wasd("d") }
Btn { lbl: "D"; onTapped: _wasd("s") }
```

**Step 4 — The Auto toggle button remains unchanged:**
```qml
ToggleBtn {
    onTapped: {
        root.motorAuto = !root.motorAuto;
        _motor(root.motorAuto ? "auto" : "unauto");
        // Explicit toggle → send immediately, no timer
        motorAutoTimer.stop();   // cancel any pending timer from WASD
    }
}
```

**Packet sequence example (user presses U three times quickly, then waits):**
```
t=0ms:   send "w"        (direction)
         motorAuto → false (UI)
         timer restarted
t=100ms: send "w"
         timer restarted
t=200ms: send "w"
         timer restarted
t=1200ms: timer fires → send "unauto"
```

Only one `"unauto"` packet per burst — correct.

**Packet sequence example (user clicks Auto ON while timer is pending):**
```
t=0ms:   _wasd("w") → send "w", motorAuto=false, timer starts
t=500ms: user clicks Auto toggle → motorAuto=true, send "auto", timer.stop()
         (timer cancelled — no spurious "unauto" after "auto")
```

---

## F-2 Camera Reconnect on Online → Offline → Online Transition

### When
A camera loses its RTSP stream (network issue, camera reboot) → the server
marks it offline in the next CAMERA packet → the UI shows OFFLINE. Then the
camera comes back → the server marks it online again → the UI should resume
streaming **automatically** without requiring app restart or manual action.

### Where

**Relevant files and classes:**
- `Qt/Back/CameraModel.cpp` — `onStoreUpdated()` updates `isOnline`
- `Qt/Front/Component/camera/CameraCard.qml` — `Loader { active: root.isOnline && root.rtspUrl !== "" }`
- `Qt/Front/Component/VideoSurface.qml` — connects `VideoWorker::frameReady` signal
- `Qt/Back/VideoWorker.hpp/.cpp` — `startStream()` / `stopStream()`
- `Qt/Back/VideoManager.cpp` — `registerSlots()` / `registerUrls()`
- `Src/Network/Video.cpp` — `decodeLoopFFmpeg()` — does not reconnect on error

### Why the reconnect currently fails

**Problem 1 — `VideoWorker` / `Video.cpp` does not retry on stream failure:**

`Video::decodeLoopFFmpeg()` runs one decode loop. If the RTSP connection drops
(`av_read_frame` returns an error), the loop exits:
```cpp
} else {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // ← on error, the function continues sleeping and looping BUT
    //   avformat_open_input already failed or the socket closed.
    //   After the socket is closed, av_read_frame returns AVERROR_EOF or
    //   a network error code continuously → tight-loop consuming 100% CPU
    //   or the loop exits via goto cleanup_and_exit.
}
```
Once the loop exits and `cleanupFFmpeg()` is called, the `VideoWorker` holds
a `std::unique_ptr<Video>` that has finished. No reconnect attempt is ever made.

**Problem 2 — `CameraCard` Loader re-activates but worker is dead:**

`CameraCard.qml`:
```qml
Loader {
    active: root.isOnline && root.rtspUrl !== ""
}
```
When `isOnline` toggles `false → true`, the `Loader` is deactivated then
re-activated. `VideoSurface.qml` runs `_tryAttach()` which calls
`videoManager.getWorker(rtspUrl)`. The worker still exists in `VideoManager`
(workers are never removed, only stopped at `clearAll()`), but its internal
`Video` object has finished its decode loop. `getWorker()` returns the dead
worker — `VideoSurface.worker` is set — but no frames ever arrive because
`Video.m_decodeThread` has already joined.

**Problem 3 — `VideoManager` never restarts a worker:**

`registerSlots()` / `registerUrls()` only adds new workers; it never calls
`startStream()` on an existing worker:
```cpp
if (url.isEmpty() || m_workers.contains(url))
    continue;  // ← if worker exists, skip — no restart
```

### How — Fix (three-part)

#### Fix Part 1 — `Video.cpp`: reconnect loop

Wrap the outer decode attempt in a `while (!m_stopThread)` retry loop. On
connection failure, wait a configurable delay before retrying:

```
// Pseudocode — in Video::decodeLoopFFmpeg():
while (!m_stopThread) {
    // --- attempt to open + decode ---
    bool ok = tryOpenAndDecode(url);   // existing logic moved here
    if (m_stopThread) break;
    if (!ok) {
        fprintf(stderr, "[Video] stream lost, retrying in 3s: %s\n", url);
        for (int i = 0; i < 30 && !m_stopThread; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    cleanupFFmpeg();   // clean up before next attempt
}
```

Key rules:
- `cleanupFFmpeg()` must run before every retry (free codec context, format
  context, sws context).
- The retry delay must check `m_stopThread` in small intervals (100 ms) so
  that `stopStream()` can join promptly.
- `Config.hpp` should expose `VIDEO_RECONNECT_DELAY_MS = 3000`.

#### Fix Part 2 — `VideoManager`: restart dead worker on `isOnline` change

Add a new method `Q_INVOKABLE void restartWorker(const QString &rtspUrl)`:

```cpp
// VideoManager.cpp
void VideoManager::restartWorker(const QString &rtspUrl) {
    auto it = m_workers.find(rtspUrl);
    if (it == m_workers.end()) return;
    VideoWorker *w = it.value();
    w->stopStream();
    w->startStream();   // re-launches Video::decodeLoopFFmpeg on a new thread
    emit workerRegistered(rtspUrl);   // re-notifies VideoSurface to re-attach
}
```

#### Fix Part 3 — `CameraModel::onStoreUpdated`: detect Online transition and notify

When `isOnline` changes from `false` to `true` for an existing entry, emit a
new signal `cameraOnline(QString rtspUrl)`:

```cpp
// CameraModel.hpp — add signal:
signals:
    void cameraOnline(QString rtspUrl);   // emitted when isOnline false→true

// CameraModel.cpp — in the update loop inside onStoreUpdated():
bool wasOnline = entry.isOnline;
if (!wasOnline && snap.isOnline) {
    entry.isOnline = true;
    changedRoles << IsOnlineRole;
    emit cameraOnline(entry.rtspUrl);   // ← trigger reconnect
}
```

#### Fix Part 4 — `Core.cpp`: wire `cameraOnline → VideoManager::restartWorker`

```cpp
// Core::wireSignals() — add:
QObject::connect(
    m_cameraModel, &CameraModel::cameraOnline,
    m_videoManager, &VideoManager::restartWorker);
```

**Complete transition sequence after fix:**

```
[Server CAMERA packet — camera comes back online]
   → CameraStore::updateFromJson (ThreadPool)
   → CameraModel::onStoreUpdated (GUI thread)
        isOnline: false → true detected
        → dataChanged(IsOnlineRole)     CameraCard Loader re-activates
        → emit cameraOnline(rtspUrl)
             → VideoManager::restartWorker(rtspUrl)
                  → VideoWorker::stopStream()   (joins dead thread cleanly)
                  → VideoWorker::startStream()  (new thread, fresh FFmpeg context)
                  → emit workerRegistered(rtspUrl)
                       → VideoSurface._onRegistered() (if waiting)
                       → VideoSurface._tryAttach()
                       → root.worker = worker    (live worker re-attached)
                       → frameReady() arrives → update() → GPU renders frame
```

**Config addition:**
```cpp
// Src/Config.hpp — add:
constexpr int VIDEO_RECONNECT_DELAY_MS = 3000;   // retry interval after stream loss
```

---

## Summary Table — File Changes Required

| File | Change | Fixes |
|------|--------|-------|
| `Qt/Front/View/MyPage.qml` | Replace `colorSecondary/colorBackground/colorPrimary/fontColorSecondary` with correct `Theme.*` names | B-1 |
| `Qt/Front/Layout/DeviceListLayout.qml` | Remove `slotId: model.slotId` and `cropRect: model.cropRect` from `CameraCard` (use safe defaults) | B-2, B-3 |
| `Qt/Back/CameraModel.hpp` | Add `Q_INVOKABLE int slotIdForUrl(const QString &rtspUrl) const`; add `signal cameraOnline(QString rtspUrl)` | B-2 opt, F-2 |
| `Qt/Back/CameraModel.cpp` | Implement `slotIdForUrl`; detect `false→true` isOnline transition; emit `cameraOnline` | B-2 opt, F-2 |
| `Qt/Front/View/DashboardPage.qml` | In `onDragEndedOutside`, call `ghost.Drag.drop()` before boundary check | B-4 |
| `Qt/Front/Component/device/DeviceControlBar.qml` | Add `Timer motorAutoTimer`; add `_wasd(dir)` function; change 4 direction buttons to call `_wasd` | F-1 |
| `Qt/Back/VideoManager.hpp` | Add `Q_INVOKABLE void restartWorker(const QString &rtspUrl)` | F-2 |
| `Qt/Back/VideoManager.cpp` | Implement `restartWorker`: stop → start existing worker; emit `workerRegistered` | F-2 |
| `Src/Network/Video.cpp` | Wrap decode attempt in retry loop; sleep `VIDEO_RECONNECT_DELAY_MS` between attempts | F-2 |
| `Core.cpp` | Wire `cameraModel::cameraOnline → videoManager::restartWorker` | F-2 |
| `Src/Config.hpp` | Add `VIDEO_RECONNECT_DELAY_MS = 3000` | F-2 |
