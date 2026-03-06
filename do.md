# Work Log: Environment Fix for `cmake --preset anomap-ninja-release`

## Request Scope I Followed
- Ran the project configure using `cmake --preset anomap-ninja-release`.
- Did not edit any build folder contents.
- Did not edit `build-anomap` scripts.
- Changed only base project config file(s).

## What I Changed
- Edited `CMakePresets.json` only.

### 1) vcpkg environment fix
- In `configurePresets.base.environment`:
  - Changed `VCPKG_FORCE_SYSTEM_BINARIES` to `null` (unset).
  - Kept `VCPKG_DISABLE_METRICS` enabled.

Reason:
- vcpkg was failing with:
  - `Could not fetch powershell-core ... available by unsetting %VCPKG_FORCE_SYSTEM_BINARIES%`
- Setting `"0"` was not enough in this setup; it needed to be unset.

### 2) Ninja tool path + compiler path fix
- In `configurePresets.ninja-base.cacheVariables`, set:
  - `CMAKE_MAKE_PROGRAM` to:
    - `C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe`
  - `CMAKE_C_COMPILER` to:
    - `C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/cl.exe`
  - `CMAKE_CXX_COMPILER` to the same `cl.exe` path.

Reason:
- Initial configure error included missing Ninja/active compiler environment.
- Pinning these in the preset removes dependence on shell setup for this preset.

### 3) Clear stale generator cache values for Ninja
- In `configurePresets.ninja-base.cacheVariables`, added:
  - `CMAKE_GENERATOR_INSTANCE: ""`
  - `CMAKE_GENERATOR_PLATFORM: ""`
  - `CMAKE_GENERATOR_TOOLSET: ""`

Reason:
- Configure was failing with Ninja-specific errors caused by stale generator metadata:
  - `Ninja does not support instance specification`
  - `Ninja does not support platform specification`

## Validation I Ran
- Re-ran:
  - `cmake --preset anomap-ninja-release`

## Outcome
- Environment/bootstrap problems were fixed:
  - vcpkg started correctly
  - compiler was detected
  - dependencies were discovered
- New remaining failure is later in project/dependency generation (not base environment setup):
  - `ninja: error: failed recompaction: Permission denied`
  - from `build/anomap-ninja-release/_build/qmsetup`

## Files Modified
- `CMakePresets.json`
- `do.md` (this file)

