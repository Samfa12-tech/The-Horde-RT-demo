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

Android build integration is still pending and described in `src/platform/android/README.md`.
