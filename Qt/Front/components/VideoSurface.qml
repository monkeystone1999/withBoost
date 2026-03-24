import QtQuick
import QtMultimedia
import AnoMap.back

// Wraps VideoOutput so that cropRect (normalised 0..1) can crop the video.
// Strategy: scale the VideoOutput up to (1/cropRect.w, 1/cropRect.h) of the
// container size and offset it so only the desired tile region is visible.
// The container clips the overflow.
// When cropRect == Qt.rect(0,0,1,1) the VideoOutput exactly fills this item.
Item {
    id: rootItem

    property int slotId: -1
    property string cameraId: ""

    // Normalised crop rect [0..1] from CameraModel.cropRect (QRectF).
    // Default: full frame (no crop).
    property rect cropRect: Qt.rect(0, 0, 1, 1)

    clip: true

    VideoOutput {
        id: videoOutput

        // Scale up so that the visible crop window fills rootItem's size.
        // For cropRect.width=0.5 we need the VideoOutput to be 2x wider
        // so that the desired half maps exactly to rootItem.width.
        width: cropRect.width > 0 ? rootItem.width / cropRect.width : rootItem.width
        height: cropRect.height > 0 ? rootItem.height / cropRect.height : rootItem.height

        // Shift so the top-left of the crop starts at (0,0) of rootItem.
        x: cropRect.width > 0 ? -cropRect.x * (rootItem.width / cropRect.width) : 0
        y: cropRect.height > 0 ? -cropRect.y * (rootItem.height / cropRect.height) : 0

        // Stretch: the video exactly fills the VideoOutput rectangle,
        // then we pan/zoom via x/y/width/height above.
        fillMode: VideoOutput.Stretch

        VideoStream {
            id: backendStream
            slotId: rootItem.slotId
            cameraId: rootItem.cameraId

            // Feed the QVideoSink from VideoOutput into our C++ VideoStream
            videoSink: videoOutput.videoSink
        }
    }
}
