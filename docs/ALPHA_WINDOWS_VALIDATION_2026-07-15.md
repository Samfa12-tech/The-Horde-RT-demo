# Initial alpha Windows validation - 2026-07-15

## Exact package

- Candidate: `Horde-Lantern-RT-Alpha-0.1.0-alpha.1-Windows-x64.zip`
- SHA-256: `1bae34e6d323bbd201ff4dd113f3c78518788f90a298a7babce1a446da2721cc`
- Itch channel: `samfa12/the-horde:windows-x64`
- Itch upload/build: `#18339908` / `#1798649`
- User version: `0.1.0-alpha.1`
- Validated GPU: NVIDIA GeForce RTX 5050 Laptop GPU

## Build and package checks

- CMake Release built `HordeLanternRT.exe` and the combat smoke test.
- Combat smoke passed swing, hit/death, respawn, and enemy-damage pulse.
- The executable uses the static MSVC runtime; Vulkan remains a driver prerequisite.
- The zip contains 21 files under an executable-relative tree: executable, controls/readme, current release notes, `ASSET_LICENSES.md`, skeleton GLB, three raw PBR arrays, and thirteen FilmCow WAVs.
- A clean extraction launched using packaged assets rather than the source-tree fallback.
- The packaged report selected `RayTracingPipeline`, recorded `RT scene presented: yes`, and contained live internal resolution, FPS, and frame-time values.

## Runtime evidence

- Default 100% RT target: `982x628`.
- Live packaged report: `163.24 FPS / 6.13 ms` in the captured scene state.
- Saved 75% setting recreated the RT target at `737x471`, retained warm colour, and honestly re-presented.
- Entry, settings, live scene, and diagnostics were captured from the packaged build.

Evidence:

- `docs/validation/request-audit-2026-07-15/01-entry-menu.png`
- `docs/validation/request-audit-2026-07-15/02-settings.png`
- `docs/validation/request-audit-2026-07-15/03-live-scene.png`
- `docs/validation/request-audit-2026-07-15/04-diagnostics.png`

## Remaining extended desktop sweep

The alpha package and core settings/diagnostics path are verified. A later compatibility pass should still repeat explicit 100% and 150% Windows display scaling.

The refreshed candidate adds an embedded Per-Monitor V2 application manifest, DPI-scaled fonts/overlay geometry, minimum window sizing, `WM_DPICHANGED` handling, and an in-app Credits & Licences dialog. On 2026-07-16 it passed a freshly extracted live sweep at the machine's active 125% scale: entry/pause, settings, 90% render scale, diagnostics, F1/F2, credits, fullscreen, maximize/restore, minimum resize, combat input, and quit. The sweep caught and fixed a clipped branded title by widening the shared menu/settings panel before the final rebuild.
