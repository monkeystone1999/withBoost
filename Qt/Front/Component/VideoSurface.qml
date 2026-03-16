import QtQuick
import QtMultimedia
import AnoMap.back

// ── VideoSurface ─────────────────────────────────────────────────────────────
// Wraps VideoOutput so that cropRect (normalised 0..1) can crop the video.
// Strategy: scale the VideoOutput up to (1/cropRect.w, 1/cropRect.h) of the
// container size and offset it so only the desired tile region is visible.
// The container clips the overflow.
// When cropRect == Qt.rect(0,0,1,1) the VideoOutput exactly fills this item.
// ─────────────────────────────────────────────────────────────────────────────
Item {
    id: root

    property int slotId: -1
    // Legacy fallback — used when slotId is not set
    property string rtspUrl: ""

    // Normalised crop rect [0..1] from CameraModel.cropRect (QRectF).
    // Default: full frame (no crop).
    property rect cropRect: Qt.rect(0, 0, 1, 1)

    clip: true

    VideoOutput {
        id: videoOutput

        // Scale up so that the visible crop window fills root's size.
        // For cropRect.width=0.5 we need the VideoOutput to be 2x wider
        // so that the desired half maps exactly to root.width.
        width: cropRect.width > 0 ? root.width / cropRect.width : root.width
        height: cropRect.height > 0 ? root.height / cropRect.height : root.height

        // Shift so the top-left of the crop starts at (0,0) of root.
        x: cropRect.width > 0 ? -cropRect.x * (root.width / cropRect.width) : 0
        y: cropRect.height > 0 ? -cropRect.y * (root.height / cropRect.height) : 0

        // Stretch: the video exactly fills the VideoOutput rectangle,
        // then we pan/zoom via x/y/width/height above.
        fillMode: VideoOutput.Stretch

        VideoStream {
            id: backendStream
            slotId: root.slotId
            rtspUrl: root.rtspUrl

            // Feed the QVideoSink from VideoOutput into our C++ VideoStream
            videoSink: videoOutput.videoSink
        }
    }
}
