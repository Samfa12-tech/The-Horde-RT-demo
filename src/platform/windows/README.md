# Windows Platform Path

Windows is the secondary/equal hardware target for this phase.

## Build and run

- Build and run the CLI capability probe:
  - `cmake -S . -B build`
  - `cmake --build build --config Debug --target horde_rt_capability_probe`
  - `.\build\Debug\horde_rt_capability_probe.exe`
- Build and run the native diagnostic window:
  - `cmake --build build --config Debug --target horde_rt_diagnostic_window`
  - `.\build\Debug\HordeLanternRT.exe`
- Reports are written to:
  - `reports/vulkan_capability_report.txt`
  - `reports/vulkan_capability_report.json`

## What is implemented

- Shared probe logic from `src/vulkan/*` and `src/ui/DiagnosticOverlay.cpp`.
- The interactive Win32 path presents the shared RT scene through a Vulkan swapchain with `WASD`, mouse-drag look, right mouse/Space swing, `Q` parry, `Esc` pause, `R` restart, `F1` controls, `F2` diagnostics, and `Alt+Enter` fullscreen. XInput and WinMM controllers are supported; the owner-tested Backbone One uses both sticks, RT attack, LT parry, B/Circle directional dodge, D-pad menus, A select, Menu/Start pause, and controller render-scale adjustment.
- The renderer builds BLAS/TLAS, dispatches `vkCmdTraceRaysKHR`, and preserves clear unsupported-device diagnostics.
- Branded entry/pause/settings/diagnostics UI, bounded two-skeleton opening combat followed by a singular lich, FilmCow SFX, positional DRAGON-STUDIO/Pixabay waterfall ambience, persisted settings, 50-100% RT render scaling, and an in-app credits/licences dialog are implemented.
- The executable declares Per-Monitor V2 DPI awareness and rescales its fonts and overlay geometry when the window DPI changes.
- Release assets resolve beside `HordeLanternRT.exe`; the portable zip uses the static MSVC runtime and leaves Vulkan as a driver prerequisite.
- Console output remains available from the CLI probe target.

## Intentionally bounded

- At most two simultaneous skeletons and one singular lich route; timed parry affects skeleton melee only, with no larger horde AI or held guard.
- No fake RT or non-Vulkan success fallback.
- The complete route includes a roof-water drench and torch failure, clear RT streams with a rounded catchment and drain runnel, blue skylight, four selected coloured-light bays, an open framed threshold, a live-lighting single-bounce hero mirror, and a misted staff-lit lich/sliding-roof finale. The rejected stained pane remains absent; broad fluid simulation is deferred.

Windows remains the equal validation target, but phone-safe renderer and asset decisions remain authoritative.
