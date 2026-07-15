# Initial alpha Windows validation - 2026-07-15

## Exact package

- Candidate: `Horde-Lantern-RT-Alpha-0.1.0-alpha.1-Windows-x64.zip`
- SHA-256: `11b59046d531a1d3f86c3f5ae488ee6dc26a5deeaf2e39e11da304575edd2f19`
- Itch channel: `samfa12/the-horde:windows-x64`
- Itch upload/build: `#18339908` / `#1797811`
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

The alpha package and core settings/diagnostics path are verified. A later polish pass should still repeat every menu action, F1/F2, fullscreen, arbitrary resize, minimize/restore, look sensitivity, and 100%/150% Windows display scaling as one uninterrupted interaction sweep.
