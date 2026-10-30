# Horde Lantern RT

A native Vulkan hardware-ray-tracing game/tech demo for **Android and Windows**, with a historical-gothic tomb, sword combat, physical lanterns, water, mirrors and a Keeper finale.

## Play and current development

- **Public release: 1.6.2.** Download from [itch.io](https://samfa12.itch.io/the-horde) or [GitHub v1.6.2](https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.6.2). Android versionCode 10; frozen release source `db62032d`. [Package hashes, verified downloads and release limits](docs/evidence/2026-10-09-release-publication/README.md).
- **Main contains unfinished 1.7 development.** [PR27](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/27) merged on 30 October 2026 at `f0cea47b`; it is no longer a draft. This is source integration, **not a 1.7 release**. World/assets, reporting and full-platform acceptance remain unfinished; experimental feature gates remain OFF. See the [current design and acceptance index](docs/design/HORDE_1_7_CURRENT_DESIGN_INDEX.md).
- **Defaults depend on the version.** Published 1.6.2 starts/reset at 50% mobile scale with Mist On; 1.7 development starts/reset at 33% with Mist Off. Explicit saved choices are preserved. Older 75% recommendations are historical, not current defaults.
- **Performance is not solved.** No sustained mobile 30 FPS claim is made. For 1.7, measured optimisation continues without a fixed-FPS release veto or reducing approved scope; correctness and release approval still matter. See the [owner decision](PROJECT_DECISIONS.md#phone-performance-and-17-scope---2026-10-26) and [experiment ledger](docs/PERFORMANCE_EXPERIMENT_LEDGER.md).

## Hardware and controls

Real Vulkan hardware RT is required: acceleration structures, hardware ray queries, buffer device address and required texture/features. The preferred Pipeline backend additionally requires RT-pipeline/SBT support; RayQueryCompute uses genuine BLAS/TLAS-backed hardware queries. There is **no raster, software-traversal or fake-RT fallback**. Unsupported devices show diagnostics.

Exact Samsung SM-S948B and Windows RTX 5050 Laptop builds have recorded evidence. Other phones, drivers and releases have different coverage: consult the [device compatibility record](docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md), not a chipset name or Android packaging minimum. Build/host tests do not certify physical RT or sustained performance.

Use the in-game **Controls** screen for the selected build. Android supports touch movement/look and contextual actions; Windows supports keyboard/mouse. Controller mappings and the specific owner-tested Backbone BB-51 scope are in the [controller record](docs/ENGINEERING_1_6_2_ANDROID_CONTROLLER_2026_10_08.md).

## Build and run

From the repository root on Windows, with the required native toolchain:

```powershell
.\tools\restore-report-webview2.ps1
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
```

The WebView2 restore verifies the pinned native SDK; configure does not download it. Run the resulting `HordeLanternRT.exe` with its required runtime assets. Offline report export remains available without web verification.

Android build only (does not install or clear a device):

```powershell
cd android
.\gradlew.bat assembleDebug --console=plain
```

See the [validation guide](docs/AGENT_VALIDATION.md) for host-only CI commands, native presets, asset/package requirements, device evidence and targeted checks. Signing, publishing and production installation are separate actions; follow the [owner release safeguards](docs/OWNER_RELEASE_SAFETY_CHECKLIST.md). Existing published packages must not be rebuilt or replaced.

## Documentation

| Start here | Contents |
| --- | --- |
| [Experiment ledger](docs/PERFORMANCE_EXPERIMENT_LEDGER.md) | What worked, what did not, what is deferred, exact cohorts and evidence availability |
| [Roadmap](docs/ROADMAP.md) / [source inventory](docs/ROADMAP_SOURCES.md) | Approved scope, later chapters, plans and historical records |
| [Current 1.7 index](docs/design/HORDE_1_7_CURRENT_DESIGN_INDEX.md) | Implemented, gated and unfinished work; scoped acceptance |
| [Project decisions](PROJECT_DECISIONS.md) / [agent instructions](AGENTS.md) | Product and engineering contracts |
| [1.6.2 validation](docs/SHOWCASE_ALPHA_1_6_2_RELEASE_VALIDATION_2026-10-09.md) | Frozen public package evidence and limitations |
| [Historical README archive](README_HISTORY_2026-10-30.md) | Complete pre-cleanup README: older controls, architecture, commands and release history, not current guidance; [historical 1.6.0 release evidence](docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md) |

## Licence

Original Horde engine/game code is [MIT licensed](LICENSE). This is a mixed-licence repository: assets, media, branding, campaign content, Pocket Audio Core and other separately licensed material are **not** covered by that grant. Read [licence scope](LICENSE_SCOPE.md) and [asset licences](ASSET_LICENSES.md).
