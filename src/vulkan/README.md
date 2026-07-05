# Vulkan Layer

The Vulkan layer will own:

- Vulkan instance creation.
- Physical device enumeration.
- Extension and feature-chain queries.
- Logical device creation with required RT features.
- Swapchain setup later.
- Capability report data.

Current files are scaffolding only. The real next task is to wire `vkEnumeratePhysicalDevices`, extension enumeration, and feature queries.

The required Phase 0 report fields are listed in `docs/TECHNICAL_REQUIREMENTS.md`.
