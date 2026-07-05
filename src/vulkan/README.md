# Vulkan Layer

The Vulkan layer now performs the real Phase 0A hardware RT capability probe.

Responsibilities:

- Vulkan instance creation.
- Physical device enumeration and selection.
- Device extension enumeration.
- `vkGetPhysicalDeviceFeatures2` chain queries.
- RT mode evaluation and diagnostics.
- Report struct population.

Current outputs:

- Plain text: `reports/vulkan_capability_report.txt`
- JSON: `reports/vulkan_capability_report.json`

Android integration is now available through `android/app/src/main/cpp/android_probe_bridge.cpp` and `android/app/src/main/java/com/samfa12/hordelanternrt/*`.