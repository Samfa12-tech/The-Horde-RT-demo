# App Layer

The app layer owns the high-level Phase 0 flow:

1. Start native platform entry point.
2. Initialise Vulkan capability-probe path.
3. Query device capabilities.
4. Build diagnostic overlay data.
5. Write JSON/text capability report.
6. Show unsupported screen if required.

Current files are skeletal and do not yet perform real Vulkan startup.
