# Install Checklist

Phase 0 needs a clean native Vulkan development environment for both Windows and Android.

## Windows host tools

- Windows 11.
- Git.
- Visual Studio 2022 with **Desktop development with C++**.
- CMake.
- Ninja.
- Vulkan SDK.
- RenderDoc.
- NVIDIA Nsight Graphics.
- Vulkan Hardware Capability Viewer or equivalent.

## Android tools

- Android Studio.
- Android SDK.
- Android NDK.
- Android CMake package.
- USB debugging enabled on the Samsung Galaxy S26 Ultra.
- Vulkan-capability checker available on the phone, if useful.
- Snapdragon Profiler, if useful and compatible with the device/toolchain.

## Target hardware

- Samsung Galaxy S26 Ultra.
- Windows laptop with RTX 5050.

## Phase 0 setup checks

1. Clone the repo.
2. Confirm CMake can configure the scaffold.
3. Confirm the Vulkan SDK is discoverable on Windows.
4. Confirm Android Studio can see the Android SDK, NDK, and CMake package.
5. Confirm the phone is visible through ADB.
6. Confirm a Vulkan-capability tool can show the phone GPU and Vulkan API version.
7. Do not begin gameplay work until native Vulkan RT capability probing is implemented.

## Expected early limitation

The scaffold CMake target is not a runnable game and not a working RT renderer. It only establishes the project shape for the real capability probe.
