# Refactoring Tasks

1. **Improve Core and App**: Since the contents of the `Src` folder have changed significantly, improve `Core.cpp`, `Core.hpp`, and `App.cpp`.
2. **Refactor Device/Camera Dependencies**: Refactor device and camera information, previously managed in `Qt/Back`, to receive dependencies from the `Src` layer via pointers. Preserve functional details as specified below.
3. **Dashboard CameraCard Logic**: Maintain existing `CameraCard` functionalities in `Qt/Front/pages/DashboardPage.qml`. Since core logic resides in `Src`, define UI-centric data (Split, Merge, Swap, newWindow) in `Qt/Back` under new names. Avoid internal use of `CameraID` unless absolutely necessary.
4. **AI Page Image Mapping**: The `id: latestAiImage` component in `Qt/Front/pages/AiPage.qml` must utilize the `CameraImage` struct from `Src/Domain/CameraManager.hpp`. Adjust dependencies to allow referencing `CameraInfo`.
5. **Device Page LiveGraph Values**: `LiveGraph` values in `Qt/Front/pages/DevicePage.qml` must reflect `CameraMeta` or `CameraStatus` from `Src/Domain/CameraManager.hpp`. Update dependencies accordingly.
6. **Decouple VideoStream**: In `Qt/Back/Services/VideoStream.hpp`, remove direct includes of `Video` from the `Src` folder. Isolate them and use dependency injection to access the stream via `std::unique_ptr<VideoEngine>` within `CameraInfo` from `Src/Domain/CameraManager.hpp`.
7. **Qt/Front Folder Naming**: Ensure all folder names within `Qt/Front` start with an uppercase letter. Convert all plural folder names to singular (remove 's' or 'es' suffixes).
8. **Cleanup and Linking Audit**: Delete all obsolete files and perform a comprehensive audit of all `CMakeLists.txt` files to verify linking.
