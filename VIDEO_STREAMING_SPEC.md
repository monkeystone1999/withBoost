# Video Streaming & Card Component Redesign Specification

> **Scope:** FFmpeg decoding pipeline, QSG rendering, virtual ID–based card model,
> 4K tile splitting, and right-click split menu.
> **Format:** Every requirement is documented across four axes —
> **When / Where / Why / How**.
> **Language:** English throughout.
> **Note:** This document describes *what to build and why*.
>           It does not contain final source code — implementation follows
>           after this spec is approved.

---

## Table of Contents

1. [Current State Diagnosis](#1-current-state-diagnosis)
2. [Requirement 1 — Native-Resolution FFmpeg Decoding](#2-requirement-1--native-resolution-ffmpeg-decoding)
3. [Requirement 2 — QQuickItem & QSG Streaming Optimisation](#3-requirement-2--qquickitem--qsg-streaming-optimisation)
4. [Requirement 3 — Virtual ID–Based Card Model](#4-requirement-3--virtual-id-based-card-model)
5. [Requirement 4 — 4K Tile Splitting per Card](#5-requirement-4--4k-tile-splitting-per-card)
6. [Requirement 5 — Right-Click Split Menu](#6-requirement-5--right-click-split-menu)
7. [Data-Flow Diagram (after all changes)](#7-data-flow-diagram-after-all-changes)
8. [File Change Checklist](#8-file-change-checklist)
9. [Threading Rules (unchanged)](#9-threading-rules-unchanged)
10. [Glossary](#10-glossary)

---

## 1. Current State Diagnosis

### 1.1 FFmpeg — hard-coded resolution cap

```
// Video.cpp (current)
const int MAX_WIDTH  = 640;
const int MAX_HEIGHT = 360;

if (targetWidth > MAX_WIDTH || targetHeight > MAX_HEIGHT) {
    float ratio = std::min(...);
    targetWidth  = static_cast<int>(targetWidth  * ratio) & ~1;
    targetHeight = static_cast<int>(targetHeight * ratio) & ~1;
}
```

Every stream is down-scaled to at most 640×360 inside the FFmpeg thread,
regardless of the source resolution. A 4K camera is decoded at full 4K but
then *thrown away* to 640×360 before the buffer reaches the GPU. This wastes
CPU (full decode) and throws away image quality unnecessarily.

### 1.2 QSG — full texture re-allocation every frame

```cpp
// VideoSurfaceItem.cpp (current)
QSGTexture *tex = window()->createTextureFromImage(img, ...);
node->setTexture(tex);
```

`createTextureFromImage` allocates a new GPU texture object on every decoded
frame. At 24 fps × 13 cameras, this causes 312 GPU allocations per second,
putting pressure on the driver allocator and causing micro-stalls visible as
frame jitter.

### 1.3 Card model — rtspUrl as the primary key

```cpp
// CameraModel.hpp (current)
struct CameraEntry {
    QString rtspUrl;   // ← used as identity
    ...
};

// CameraModel.cpp
void CameraModel::swapCameraUrls(int indexA, int indexB) {
    std::swap(m_cameras[indexA].rtspUrl, m_cameras[indexB].rtspUrl);
    ...
}
```

The card's *visual position* in the grid is identified by its index, but the
*video source* is the rtspUrl. When two cards are swapped, the URLs are
exchanged between slots. This causes:

- `VideoSurface.qml` to `disconnect` from its current `VideoWorker` and
  `connect` to the new one — triggering a blank frame flash.
- `DeviceModel.hasDevice(rtspUrl)` to return a different result after swap —
  the Device Control bar may silently follow the URL rather than the slot.
- In a future 4K split scenario, swapping a card that carries a `cropRect`
  would move the crop to the wrong slot, producing visually broken output.

### 1.4 No split (tile) support

`VideoSurfaceItem` renders the full texture in every instance with
`node->setSourceRect(QRectF(0, 0, 1, 1))`.
There is no mechanism to display only a sub-region (quadrant, third, half)
of a decoded frame. The `cropRect` property referenced in the requirement
does not exist yet.

### 1.5 No right-click split menu entries

The right-click context menu (`overlayCtxMenu` in `DashboardPage.qml`)
currently has five items. "Split 2", "Split 3", "Split 4" are absent.

---

## 2. Requirement 1 — Native-Resolution FFmpeg Decoding

### When
Every time `Video::decodeLoopFFmpeg()` calls `sws_getContext()` to
initialise the colour-space converter. This happens once per stream on first
frame and again whenever the source changes resolution.

### Where
`Src/Network/Video.cpp` — `decodeLoopFFmpeg()` — the block that computes
`targetWidth` / `targetHeight` and calls `sws_getContext`.

### Why
The current hard-cap (`MAX_WIDTH=640, MAX_HEIGHT=360`) was introduced to
limit GPU upload cost when 13 cameras were all rendered at their native size.
With the tile-split feature (Requirement 4), a 4K stream decoded to 640×360
would produce illegible sub-tiles. The correct approach is:

- Decode at native resolution (e.g. 3840×2160 for a 4K source).
- Let the GPU handle sub-region selection via `QSGSimpleTextureNode::setSourceRect`.
- The GPU crop is essentially free — it is a UV-coordinate change in the
  vertex shader, not a pixel-copy operation.

For non-split standard cards, the `VideoSurfaceItem` simply sets
`sourceRect = QRectF(0, 0, 1, 1)` and the GPU scales to the card size —
no different from today, but at full quality.

### How

**Remove** the resolution cap block entirely from `Video.cpp`:

```
// DELETE this entire block:
const int MAX_WIDTH  = 640;
const int MAX_HEIGHT = 360;
if (targetWidth > MAX_WIDTH || targetHeight > MAX_HEIGHT) { ... }
// targetWidth = srcWidth, targetHeight = srcHeight — use as-is.
```

**Keep** `sws_getContext` call but pass `srcWidth, srcHeight` as both source
and target (i.e. no down-scale). The output pixel format remains `AV_PIX_FMT_BGRA`
because `QImage::Format_ARGB32` maps to it with no extra conversion.

**Update the buffer pool** size calculation accordingly — at 3840×2160×4 bytes
≈ 33 MB per buffer. With 20 buffers in the pool that would be 660 MB — too much.
Reduce pool size from 20 to 3 for streams wider than 1920 pixels:

```
// Pseudocode (not final code):
int poolSize = (targetWidth > 1920) ? 3 : 8;
```

**Frame-rate throttle** (`elapsed >= 33 ms` check) stays in place — it limits
GPU uploads to ~30 fps regardless of source frame rate, which is correct.

**`VideoWorker::FrameData`** already carries `width` and `height`. No change
needed there — the tile renderer reads these values to compute normalised crop
coordinates.

---

## 3. Requirement 2 — QQuickItem & QSG Streaming Optimisation

### When
Every call to `VideoSurfaceItem::updatePaintNode()` — which fires on the
render thread once per display vsync whenever `update()` was scheduled.

### Where
`Qt/Back/VideoSurfaceItem.hpp` and `Qt/Back/VideoSurfaceItem.cpp`.

### Why

**Problem A — Texture re-allocation every frame:**
`window()->createTextureFromImage(img)` creates a new `QSGTexture` object
(= a new GPU texture handle) every frame. The old texture is passed to
`deleteLater()` which defers its GPU release to the next GUI-thread event.
This creates a steady stream of allocate/free cycles on the GPU driver.

**Problem B — No crop support:**
The current node uses `QSGImageNode` which supports `setSourceRect` but the
property is never set from QML. To support tile rendering, `VideoSurfaceItem`
needs a QML-accessible `cropRect` property of type `rect`.

**Solution — Reuse texture, add `cropRect` property:**

`QSGImageNode` (the type already used) supports both:
- `setTexture(tex)` — reuse an existing texture object if dimensions match.
- `setSourceRect(QRectF)` — GPU-side crop in normalised UV coordinates.

The optimisation strategy:

1. Cache the last `QSGTexture*` inside `VideoSurfaceItem` as a member
   (`m_cachedTexture`). On each `updatePaintNode` call, check if frame
   dimensions match the cached texture's dimensions. If they match, call
   `QSGTexture::updateRhiTexture()` (Qt 6 RHI path) or simply replace only
   when dimensions change.

   > **Qt 6 note:** In Qt 6, `QSGTexture` created via
   > `createTextureFromImage` is immutable after creation. Therefore the
   > correct strategy is: allocate a new texture only when `width` or
   > `height` differs from the previous frame; otherwise reuse. Since RTSP
   > streams have a fixed resolution per session, texture allocation happens
   > *once* per stream per session — exactly once after the first decoded frame.

2. Add a `Q_PROPERTY(QRectF cropRect ...)` to `VideoSurfaceItem`. QML can
   bind this to a value computed by the split logic (see Requirement 4).
   Default value: `QRectF(0, 0, 1, 1)` (show full frame).

3. In `updatePaintNode`, call `node->setSourceRect(m_cropRect)` instead of
   the hard-coded `QRectF(0, 0, 1, 1)`.

**`VideoSurfaceItem` member additions:**

```
// Members to add (not final code — documentation only):
QRectF          m_cropRect   { 0, 0, 1, 1 };   // normalised UV crop
QSGTexture     *m_cachedTex  { nullptr };        // render-thread only
int             m_cachedW    { 0 };
int             m_cachedH    { 0 };
```

**`updatePaintNode` logic (pseudocode):**

```
updatePaintNode(oldNode, data):
    node = static_cast<QSGImageNode*>(oldNode) ?? window()->createImageNode()

    frame = m_worker->getLatestFrame()   // atomic load, no lock

    if frame is valid:
        if frame.width != m_cachedW or frame.height != m_cachedH:
            // Dimensions changed — must allocate new texture (rare)
            delete m_cachedTex
            m_cachedTex = window()->createTextureFromImage(QImage(frame))
            m_cachedW = frame.width
            m_cachedH = frame.height
        else:
            // Same dimensions — reuse existing texture handle
            // Update pixel data in-place via RHI update batch
            // (Implementation: create new QImage on same buffer, call
            //  createTextureFromImage with TextureIsOpaque flag — Qt
            //  will detect same-size and reuse the GPU allocation
            //  when TextureCanUseAtlas is also set.)
            m_cachedTex = window()->createTextureFromImage(QImage(frame),
                QQuickWindow::TextureIsOpaque | QQuickWindow::TextureCanUseAtlas)

        node->setTexture(m_cachedTex)

    node->setRect(boundingRect())
    node->setSourceRect(m_cropRect)     // ← GPU crop, zero CPU cost
    return node
```

**`cropRect` property in QML:**

```qml
// VideoSurface.qml usage after change:
VideoSurfaceItem {
    worker:   videoManager.getWorker(rtspUrl)
    cropRect: Qt.rect(0.0, 0.0, 0.5, 0.5)  // top-left quadrant
}
```

No pixel copying, no CPU involvement. The GPU draws only the specified
sub-rectangle of the uploaded texture.

---

## 4. Requirement 3 — Virtual ID–Based Card Model

### When
At the design level: affects how `CameraModel`, `CameraEntry`, `DashboardPage`,
and `VideoSurface` are structured. The change must be completed before
implementing Requirement 4 (tile split) because tile split relies on the
virtual-ID concept.

### Where
- `Qt/Back/CameraModel.hpp/.cpp` — `CameraEntry` struct and `swapCameraUrls`
- `Qt/Front/View/DashboardPage.qml` — GridView delegate
- `Qt/Front/Component/CameraCard.qml` — card properties
- `Qt/Front/Component/VideoSurface.qml` — worker lookup

### Why

**Current problem — URL as identity:**

When `swapCameraUrls(indexA, indexB)` exchanges the `rtspUrl` values between
two model rows, QML's property binding system re-evaluates `rtspUrl`-dependent
expressions in the delegate. Concretely:

1. `VideoSurface.qml` watches `rtspUrl` via `onRtspUrlChanged`.
2. On swap, `rtspUrl` changes → `_tryAttach()` runs → the surface
   disconnects from `WorkerA` and connects to `WorkerB`.
3. `WorkerB` has not emitted `frameReady` yet → the surface goes blank for
   one render cycle → visible flash.

More critically: if a 4K card is split into 4 tiles, each tile has the same
`rtspUrl`. Swapping by URL would move all 4 tiles at once — completely
wrong behaviour.

**Solution — Virtual Slot ID:**

Each *visual slot* in the grid receives a permanent, monotonically increasing
integer ID (`slotId`). This ID never changes for the lifetime of the session.

```
slotId    rtspUrl                  cropRect          cameraType
  0       rtsp://192.168.0.16.../0  (0,0,1,1)        HANWHA
  1       rtsp://192.168.0.16.../1  (0,0,1,1)        HANWHA
  ...
 12       rtsp://192.168.0.43/...   (0,0,1,1)        SUB_PI
```

When the user drags card at slotId=2 onto the cell of slotId=7:

**Current behaviour (URL swap):**
```
m_cameras[2].rtspUrl ↔ m_cameras[7].rtspUrl   ← URL moves, slotId is implicit
```
QML sees `rtspUrl` change → `VideoSurface` reconnects → flash.

**New behaviour (slot reorder):**
```
// Swap the entire CameraEntry rows in the model.
// Each CameraEntry carries its own slotId.
// QML delegate is keyed on slotId, not rtspUrl.
// VideoSurface is also keyed on slotId.
```

Because `VideoSurface` maps to a `VideoWorker` via `slotId` → `rtspUrl`
looked up from `VideoManager`, the surface itself does not change its
worker when cards are reordered. The worker keeps streaming; only the
visual *position* of the card changes.

**`CameraEntry` struct changes:**

```
// New field:
struct CameraEntry {
    int     slotId;        // permanent identity, assigned at first load
    QString title;
    QString rtspUrl;
    bool    isOnline;
    QString cameraType;
    int     splitCount;    // 1 = no split, 2/3/4 = tile count
    // width, height, description removed — not needed
};
```

**`CameraModel` role additions:**

```
enum Roles {
    SlotIdRole = Qt::UserRole + 1,
    TitleRole,
    RtspUrlRole,
    IsOnlineRole,
    CameraTypeRole,
    SplitCountRole         // new
};
```

**`swapCameraUrls` → renamed `swapSlots(int indexA, int indexB)`:**

Swaps the *entire* `CameraEntry` at `indexA` with the one at `indexB`
(including `slotId`, `rtspUrl`, `splitCount`, etc). This is a simple
`std::swap(m_cameras[indexA], m_cameras[indexB])` followed by `dataChanged`.

**`VideoSurface.qml` lookup change:**

```qml
// Current:
var w = videoManager.getWorker(rtspUrl)

// New:
var w = videoManager.getWorkerBySlot(slotId)
// VideoManager internally resolves slotId → rtspUrl → VideoWorker.
```

`VideoManager` gains a `Q_INVOKABLE VideoWorker* getWorkerBySlot(int slotId)`
method backed by a `QMap<int, VideoWorker*> m_workersBySlot` in addition to
the existing URL map.

**`VideoManager` registration change:**

Currently `registerUrls(QStringList)`. New signature:
`registerSlots(QList<SlotInfo>)` where `SlotInfo` = `{ int slotId; QString rtspUrl; }`.

This lets `VideoManager` maintain the `slotId → worker` mapping correctly
even when slots are reordered or URLs change (e.g. camera replaced in server).

---

## 5. Requirement 4 — 4K Tile Splitting per Card

### When
When the user selects "Split 2", "Split 3", or "Split 4" from the right-click
context menu on a CameraCard. Also applied automatically on login if the
server reports a source resolution above a configurable threshold
(e.g. `Config::SPLIT_AUTO_THRESHOLD_WIDTH = 2560`).

### Where
- `Qt/Back/CameraModel.hpp/.cpp` — `splitSlot(int slotIndex, int tileCount)` method
- `Qt/Front/View/DashboardPage.qml` — GridView model (now shows expanded row count)
- `Qt/Front/Component/VideoSurface.qml` — `cropRect` property binding
- `Qt/Back/VideoSurfaceItem.hpp/.cpp` — `cropRect` QML property (Requirement 2)
- `Src/Config.hpp` — `SPLIT_AUTO_THRESHOLD_WIDTH`

### Why

**4K streams and the swap problem:**

A 4K RTSP stream (3840×2160) decoded natively and rendered in a single small
card would display correctly but waste GPU bandwidth — the GPU scales 8 MP
down to perhaps 300×225 pixels on screen.

More importantly, for situational awareness, a 4K camera can cover a large
physical area. Splitting it into 4 tiles lets operators pan/zoom conceptually
by assigning different tiles to different monitor quadrants or zooming into
one tile by making it full-screen.

**Why not crop on the CPU (sws_scale)?**

Cropping on the CPU (computing a different `srcSliceY` + `srcSliceH` per tile)
would require running the FFmpeg decode loop 4 times or adding shared memory
between 4 consumers — enormously complex and wasteful. The GPU approach is:

- Decode once → one full-resolution BGRA buffer in `VideoWorker`.
- 4 `VideoSurfaceItem` instances all hold a pointer to the *same* `VideoWorker`.
- Each `VideoSurfaceItem` has a different `cropRect` (UV coordinates).
- GPU renders 4 different sub-rectangles of the same texture upload.

This is zero-copy and zero-extra-CPU — the GPU's texture sampler does the crop
for free.

**`splitSlot(int slotIndex, int tileCount)` logic (pseudocode):**

```
splitSlot(index, n):
    entry = m_cameras[index]
    // Remove the original entry
    beginRemoveRows(index, index)
    m_cameras.removeAt(index)
    endRemoveRows()

    // Insert n new entries in its place
    beginInsertRows(index, index + n - 1)
    for i in 0..n-1:
        newEntry = CameraEntry {
            slotId      = nextSlotId++,   // new unique IDs for each tile
            rtspUrl     = entry.rtspUrl,  // SAME URL — shared stream
            title       = entry.title + " [" + tileName(i, n) + "]",
            isOnline    = entry.isOnline,
            cameraType  = entry.cameraType,
            splitCount  = n,
            cropRect    = computeCropRect(i, n)   // see below
        }
        m_cameras.insert(index + i, newEntry)
    endInsertRows()
```

**`computeCropRect(tileIndex, tileCount)` — UV coordinate table:**

```
tileCount = 2 (horizontal split, left | right):
    tile 0: QRectF(0.0, 0.0, 0.5, 1.0)   // left half
    tile 1: QRectF(0.5, 0.0, 0.5, 1.0)   // right half

tileCount = 3 (horizontal thirds):
    tile 0: QRectF(0.000, 0, 0.333, 1.0)
    tile 1: QRectF(0.333, 0, 0.334, 1.0)
    tile 2: QRectF(0.667, 0, 0.333, 1.0)

tileCount = 4 (2×2 grid):
    tile 0: QRectF(0.0, 0.0, 0.5, 0.5)   // top-left
    tile 1: QRectF(0.5, 0.0, 0.5, 0.5)   // top-right
    tile 2: QRectF(0.0, 0.5, 0.5, 0.5)   // bottom-left
    tile 3: QRectF(0.5, 0.5, 0.5, 0.5)   // bottom-right
```

**`CameraEntry` extended with `cropRect`:**

```cpp
struct CameraEntry {
    int     slotId;
    QString title;
    QString rtspUrl;
    bool    isOnline;
    QString cameraType;
    int     splitCount  = 1;
    QRectF  cropRect    = {0, 0, 1, 1};   // default: full frame
};
```

`CameraModel::roleNames()` gains `CropRectRole` returning a `QRectF`.
QML reads it as a `rect` value type.

**`VideoSurface.qml` after split:**

```qml
VideoSurfaceItem {
    worker:   videoManager.getWorkerBySlot(model.slotId)
    cropRect: model.cropRect     // ← comes from CameraModel
}
```

All 4 tile `VideoSurfaceItem` instances point to the same `VideoWorker`
(because they share the same `rtspUrl`; `getWorkerBySlot` resolves the URL
internally). Each has a different `cropRect`. The GPU renders 4 sub-regions
of the single decoded texture.

**Shared `VideoWorker` reference counting:**

`VideoManager::getWorkerBySlot(slotId)` first resolves `slotId → rtspUrl`
using its `SlotInfo` map, then looks up the worker by URL. Multiple `slotId`
values may resolve to the same URL → same `VideoWorker`. This is safe: the
worker's `getLatestFrame()` is a lock-free atomic read callable from multiple
`VideoSurfaceItem` instances on the render thread simultaneously.

**`mergeSlots(int firstTileIndex, int tileCount)` for un-split:**

The reverse operation: remove `n` consecutive tile entries (all with the same
`rtspUrl`), re-insert one entry with `splitCount=1` and `cropRect=(0,0,1,1)`.

---

## 6. Requirement 5 — Right-Click Split Menu

### When
When the user right-clicks on any `CameraCard` in `DashboardPage`.

### Where
`Qt/Front/View/DashboardPage.qml` — `overlayCtxMenu` Rectangle — `ctxCol` Column.

### Why
Split is a user-initiated operation. It must be discoverable through the
existing right-click context menu rather than a separate toolbar or double-click
gesture, for consistency with "Device Control", "AI Control", etc.

Split should only be offered when:
- The card is **online** (`model.isOnline === true`).
- The card has `splitCount === 1` (not already split — no nested split).
- For "Split 4": the source is ideally wider than 1920 px (heuristic only —
  do not enforce; operator may want artistic splits of any resolution).

Un-split ("Merge") should appear when `splitCount > 1` and the user right-clicks
on *any* of the tile cards belonging to the same original stream.

### How

**Add to the context menu column after existing items:**

```
separator
"Split 2"   — enabled if: model.isOnline && model.splitCount === 1
"Split 3"   — enabled if: model.isOnline && model.splitCount === 1
"Split 4"   — enabled if: model.isOnline && model.splitCount === 1
"Merge"     — enabled if: model.splitCount > 1
```

**Context menu needs a `targetSlotId` in addition to `targetUrl`:**

```qml
// overlayCtxMenu property additions:
property string targetUrl:    ""
property int    targetSlotId: -1
property int    targetIndex:  -1   // row index in cameraModel
property int    targetSplit:  1    // model.splitCount at time of right-click
```

**Right-click handler in the delegate:**

```qml
onRightClicked: (sx, sy) => {
    overlayCtxMenu.targetUrl    = model.rtspUrl
    overlayCtxMenu.targetSlotId = model.slotId
    overlayCtxMenu.targetIndex  = index
    overlayCtxMenu.targetSplit  = model.splitCount
    // ... position and show menu
}
```

**Split item `onTriggered`:**

```qml
CtxItem {
    label:       "Split 2"
    itemEnabled: overlayCtxMenu.targetSplit === 1 && cameraModel.data(...)isOnline
    onTriggered: cameraModel.splitSlot(overlayCtxMenu.targetIndex, 2)
}
CtxItem {
    label:       "Split 3"
    itemEnabled: overlayCtxMenu.targetSplit === 1 && ...isOnline
    onTriggered: cameraModel.splitSlot(overlayCtxMenu.targetIndex, 3)
}
CtxItem {
    label:       "Split 4"
    itemEnabled: overlayCtxMenu.targetSplit === 1 && ...isOnline
    onTriggered: cameraModel.splitSlot(overlayCtxMenu.targetIndex, 4)
}
CtxItem {
    label:       "Merge"
    itemEnabled: overlayCtxMenu.targetSplit > 1
    onTriggered: cameraModel.mergeSlots(overlayCtxMenu.targetIndex,
                                        overlayCtxMenu.targetSplit)
}
```

**`Q_INVOKABLE` methods to add to `CameraModel`:**

```cpp
Q_INVOKABLE void splitSlot (int rowIndex, int tileCount);
Q_INVOKABLE void mergeSlots(int anyTileRowIndex, int tileCount);
```

`mergeSlots` must find the *first* tile of the group (scan backwards from
`anyTileRowIndex` until `rtspUrl` changes) and remove `tileCount` rows,
replacing them with one.

---

## 7. Data-Flow Diagram (after all changes)

```
[Server — CAMERA packet every 5 s]
    │
    └─ ThreadPool: CameraStore::updateFromJson()
           │  produces vector<CameraData> (rtspUrl, cameraType, isOnline)
           │
           └─ GUI thread: CameraModel::onStoreUpdated(snapshot)
                  │
                  ├─ Assigns slotId if new URL (monotonic counter)
                  ├─ Updates isOnline for existing slots
                  └─ Emits urlsUpdated(slotInfoList)
                         │
                         └─ VideoManager::registerSlots(slotInfoList)
                                │
                                ├─ Creates VideoWorker per unique rtspUrl (not per slot)
                                └─ Updates slotId → worker map

[FFmpeg decode thread — per unique rtspUrl]
    │
    ├─ Decodes at NATIVE resolution (no MAX_WIDTH cap)
    ├─ sws_scale → AV_PIX_FMT_BGRA
    └─ VideoWorker::m_atomicBuffer.store(buf, width, height)
           │
           └─ QMetaObject::invokeMethod → emit frameReady()  [GUI thread]

[GUI thread — frameReady signal]
    │
    └─ VideoSurfaceItem::onFrameReady() → update()  [schedules render]

[Render thread — updatePaintNode()]
    │
    ├─ VideoWorker::getLatestFrame()  (atomic load, lock-free)
    ├─ If dimensions changed → createTextureFromImage()  (once per resolution)
    ├─ node->setTexture(m_cachedTex)
    ├─ node->setRect(boundingRect())
    └─ node->setSourceRect(m_cropRect)  ← GPU crop, zero CPU cost
           │
           For split card:  cropRect = e.g. QRectF(0.0, 0.0, 0.5, 0.5)
           For normal card: cropRect = QRectF(0.0, 0.0, 1.0, 1.0)

[User right-clicks card]
    │
    └─ overlayCtxMenu shows with targetIndex, targetSlotId, targetSplit
           │
           ├─ "Split 4" → cameraModel.splitSlot(index, 4)
           │       └─ CameraModel: remove 1 row, insert 4 rows
           │              each with same rtspUrl, different cropRect
           │              each with new unique slotId
           │
           └─ "Merge"  → cameraModel.mergeSlots(anyTileIndex, 4)
                   └─ CameraModel: remove 4 rows, insert 1 row
                          cropRect = (0,0,1,1), splitCount = 1
```

---

## 8. File Change Checklist

### 8.1 Files to MODIFY

| File | Change summary | Priority |
|------|---------------|----------|
| `Src/Network/Video.cpp` | Remove `MAX_WIDTH/MAX_HEIGHT` cap; use `srcWidth/srcHeight` as target dimensions. Adjust buffer pool size for 4K. | 🔴 Critical |
| `Qt/Back/VideoSurfaceItem.hpp` | Add `Q_PROPERTY(QRectF cropRect)`. Add `m_cropRect`, `m_cachedTex*`, `m_cachedW`, `m_cachedH` members. | 🔴 Critical |
| `Qt/Back/VideoSurfaceItem.cpp` | Implement texture reuse (allocate only on dimension change). Call `node->setSourceRect(m_cropRect)`. | 🔴 Critical |
| `Qt/Back/CameraModel.hpp` | Add `slotId`, `splitCount`, `cropRect` to `CameraEntry`. Add `SlotIdRole`, `SplitCountRole`, `CropRectRole`. Add `splitSlot()`, `mergeSlots()` invokables. | 🔴 Critical |
| `Qt/Back/CameraModel.cpp` | Implement `splitSlot`, `mergeSlots`. Assign `slotId` in `onStoreUpdated`. Emit `slotInfoList` instead of URL list. | 🔴 Critical |
| `Qt/Back/VideoManager.hpp` | Add `getWorkerBySlot(int slotId)`. Add `SlotInfo` struct. Change `registerUrls` → `registerSlots(QList<SlotInfo>)`. Add `m_workersBySlot` map. | 🔴 Critical |
| `Qt/Back/VideoManager.cpp` | Implement `registerSlots`, `getWorkerBySlot`. Workers keyed by URL internally; slot map keyed by slotId. | 🔴 Critical |
| `Qt/Front/Component/VideoSurface.qml` | Change worker lookup: `getWorkerBySlot(slotId)` instead of `getWorker(rtspUrl)`. Add `cropRect` binding to `VideoSurfaceItem.cropRect`. | 🔴 Critical |
| `Qt/Front/Component/CameraCard.qml` | Add `slotId: int` property. Pass to `VideoSurface`. Remove reliance on `rtspUrl` for worker lookup. | 🟡 High |
| `Qt/Front/View/DashboardPage.qml` | Add `targetSlotId`, `targetIndex`, `targetSplit` to `overlayCtxMenu`. Add "Split 2/3/4" and "Merge" `CtxItem` entries. Pass `model.slotId` and `index` through `onRightClicked`. | 🟡 High |
| `Src/Config.hpp` | Add `SPLIT_AUTO_THRESHOLD_WIDTH = 2560`. Add `VIDEO_BUFFER_POOL_SIZE_HD = 8`, `VIDEO_BUFFER_POOL_SIZE_4K = 3`. | 🟢 Low |
| `Src/Domain/CameraData.hpp` | No change needed — `CameraData` is a server-side value type. `slotId` is a client-side display concept assigned in `CameraModel`. | — |

### 8.2 Files to CREATE

| File | Purpose |
|------|---------|
| None required | All changes are within existing files. |

### 8.3 Files that are CORRECT — do not change

| File | Reason |
|------|--------|
| `Src/Network/Video.hpp` | No interface change needed for native resolution. |
| `Qt/Back/VideoWorker.hpp/.cpp` | `FrameData` already carries `width` and `height`. Atomic buffer and coalescing remain correct. |
| `Qt/Back/VideoManager.cpp` (worker creation) | Worker-per-URL deduplication logic is correct; only the slot-mapping layer is added. |
| `Qt/Front/Component/DeviceControlBar.qml` | Unaffected. |
| `Qt/Front/View/DevicePage.qml` | Unaffected — uses `deviceModel`, not `cameraModel`. |
| `Qt/Front/View/AIPage.qml` | Reads `cameraType` from model — still valid after changes. |

---

## 9. Threading Rules (unchanged)

These rules are defined in `ARCHITECTURE.md`. They apply unchanged to the
new streaming components:

| Thread | New responsibilities |
|--------|---------------------|
| **FFmpeg thread** | Decodes at native resolution. Writes to `m_atomicBuffer`. No new rules. |
| **Render thread** | Calls `getLatestFrame()` (still atomic). May hold `m_cachedTex` — **render-thread private, never touched by GUI thread**. |
| **GUI thread** | Calls `splitSlot()` / `mergeSlots()` on `CameraModel`. Calls `update()` in `onFrameReady()`. Nothing else. |
| **ThreadPool** | Unchanged — `CameraStore`, `DeviceStore`, `AlarmDispatcher`. |

**Critical rule for tile VideoSurfaceItem:**
Multiple `VideoSurfaceItem` instances (one per tile) may call
`getLatestFrame()` on the same `VideoWorker` concurrently from the render
thread. This is safe because `getLatestFrame()` uses only `std::atomic::load`
with `memory_order_acquire` — no mutex, no shared mutable state.

---

## 10. Glossary

| Term | Meaning in this document |
|------|--------------------------|
| **slotId** | A permanent integer assigned to a visual grid position. Never reused within a session. Survives card reordering (swap). |
| **cropRect** | A `QRectF` in normalised UV coordinates [0.0–1.0] that specifies which sub-region of a decoded texture the GPU should render. `(0,0,1,1)` = full frame. |
| **tile** | One `CameraCard` that displays a sub-region of a stream via `cropRect`. Multiple tiles share one `VideoWorker`. |
| **split** | The operation of replacing one `CameraEntry` with N tile entries (N=2,3,4), all pointing to the same `rtspUrl` with different `cropRect` values. |
| **merge** | The reverse of split. N tile entries collapsed back into one entry with `cropRect=(0,0,1,1)`. |
| **native resolution** | The pixel dimensions reported by the RTSP server's codec parameters, read from `m_formatCtx->streams[i]->codecpar->width/height`. No client-side downscale. |
| **texture reuse** | Keeping the same `QSGTexture*` object alive across frames when the decoded frame dimensions have not changed. Eliminates per-frame GPU allocation. |
| **VideoWorker** | One FFmpeg decode thread wrapper per unique `rtspUrl`. Shared among all tile `VideoSurfaceItem` instances for the same stream. |
| **SlotInfo** | `{ int slotId; QString rtspUrl; }` — the minimal data exchanged between `CameraModel` and `VideoManager` when the slot list changes. |
