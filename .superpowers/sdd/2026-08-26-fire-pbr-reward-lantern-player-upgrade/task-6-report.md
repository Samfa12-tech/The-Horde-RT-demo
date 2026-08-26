# Task 6 Report — Reusable Bounded Dielectric/Glass Transport

Date: 2026-08-26 (Australia/Sydney)

## Status

Task 6's reusable dielectric implementation, focused tests, exact Windows captures, fresh host gates, Android builds, lint, packaging, licence validation, and shader investigation are complete on implementation commit `e5e7592a42e92ad8b14474580e534c8facf4f999`.

The milestone's exact-phone acceptance is still open because `adb devices -l` exposed no target at the final boundary. Therefore the exact `SM-S948B` artifact-parity install, matched Mobile/High 75% glass checkpoints, separate 100% reporting, warmed timing/thermal/GPU-power/resource evidence, and Home/resume check were not performed and are explicitly carried to Task 9. No phone result is inferred from the successful Android builds.

The implementation remains the real `vkCmdTraceRaysKHR` presentation path with iterative `rayQueryEXT`, one frame in flight, strict ASTC packaging, `outputRedBlueSwap`, and a 20-instance TLAS ceiling. No pipeline recursion, screen-space refraction, alpha-only pane, copied water coordinates, quality reduction, render-scale reduction, publishing, signing, versioning, upload, or deployment occurred.

## Focused commits

- `e67a45a` — `test: add bounded dielectric transport invariants`
- `d13f656` — `refactor: share rt dielectric and terminal shading helpers`
- `2670d61` — `feat: import gltf dielectric material properties`
- `57e6833` — `feat: add iterative ray query glass transport`
- `e361a5c` — `feat: add bounded transparent shadow transmittance`
- `e5e7592` — `feat: integrate reusable dielectric fixture and diagnostics`

The final report commit is recorded in the handoff after this report is committed.

## TDD and focused contracts

Implementation followed RED/GREEN in bounded slices:

- `DielectricMathTests.cpp` first failed because `DielectricMath.h` did not exist, then passed Fresnel/Schlick, Snell refraction, entry/exit orientation and stack behavior, total internal reflection, Beer-Lambert distance attenuation, thin-wall transmission, deterministic overflow, and finite degenerate-input contracts.
- Static GLB tests first failed on the missing dielectric material fields, then passed `KHR_materials_transmission`, `KHR_materials_volume`, `KHR_materials_ior`, `KHR_materials_emissive_strength`, attenuation color/distance, thickness, transmission, IOR, roughness, and audited `thinWall` sidecar parsing.
- Topology-tool tests first failed because the validator did not exist, then passed a closed/manifold thick fixture and rejected open/non-manifold thick fixtures. Diagnostics direct the author to close/weld or repair the geometry, or use `thinWall=true` only for an intentional audited pane.
- Source/ABI tests first failed for missing bounded transport/shadow contracts, then passed exact Mobile/High budgets, terminal ownership, actual-hit-distance attenuation, opaque/metal blocker handling, diagnostic reporting, fixed ABI sizes, and append-only binding requirements.
- Development asset/slot tests first failed for a missing generic fixture route, then passed the imported static-PBR/BLAS/TLAS integration and both glass development checkpoints.

The exact fresh gate later reran these contracts in both Debug and Release as part of 26/26 CTests per configuration.

## Runtime transport

`DielectricMath.h` is the host-testable reference and the GLSL includes implement the same reusable material model. A material is selected from imported properties and flags; there is no lantern, water-object, fixture-object, or screen-position branch.

- Mobile: at most two closed dielectric volumes and four dielectric interfaces.
- High: at most four closed dielectric volumes and eight dielectric interfaces.
- Both qualities issue at most one terminal reflected ordinary opaque/emissive query.
- Transparent shadow transmittance is capped at four interfaces on Mobile and eight on High.
- Exact interface-budget hits may shade the following terminal opaque/emissive surface; another dielectric interface resolves to the deterministic bounded fallback.
- Entry/exit normals and the finite IOR stack cover nested media. Thin walls do not mutate the volume stack.
- Beer-Lambert uses accumulated actual `HitInfo.t` distance inside the medium.
- TIR uses reflection within the same bounded loop. Roughness perturbs that same bounded reflection/transmission model rather than selecting a separate cheap path.
- Reflection cannot recursively evaluate another reflective dielectric. Glass-on-glass and water-on-water over budget terminate predictably and increment the diagnostic counter.
- Shadow transport accumulates finite dielectric layers and stops on opaque or metal blockers.
- The Task 4 single-energy-ownership rule remains: reflected fire is evaluated only by the terminal reflected query and is not counted again through transmitted glossy light.

Water retains its dedicated geometry/flow coordinate identity and bounded cost, while shared Fresnel, visibility, and terminal ordinary opaque/emissive shading live in generic helpers.

## Material and descriptor ABI

`RtMaterialGpu` remains exactly 112 bytes; no existing material field moved and no material-buffer stride changed. Six imported materials occupy 672 bytes in the exact Windows capture. The generated C++ and GLSL ABI now append the dielectric fields and retain generator/freshness checks.

Existing descriptor bindings 0–21 are unchanged. Binding 22 appends one 16-byte `RtDielectricDiagnostics` storage buffer for transport/shadow overflow counters. Per-frame counter values are exposed in the Windows capture manifest and Android debug state. A rejected experiment made existing instance metadata writable; that broadened optimizer aliasing and was not retained.

The fixture adds one reusable static imported BLAS route (`kBlasCount == 12`) while the TLAS remains at or below 20 instances. It is hidden in all authored checkpoints. When explicitly enabled for RT Lab/development checkpoints it borrows the mutually exclusive procedural-player slot and forces the already-supported skinned player route, preserving the fixed capacity.

## Fixture, controls, and offline validation

The generic production fixture is:

- `assets/models/props/runtime/dielectric-fixture/closed-glass-lod0.runtime.glb`
- 1,848 bytes; SHA-256 `843fd245da61352a124e3de03f4ccf01923a293afedee2ed73b65a8a18eebf5b`
- eight vertices, 36 indices, 12 triangles; closed/manifold
- transmission 0.94, IOR 1.52, thickness 1.0 m, attenuation distance 2.4 m, colored attenuation, roughness 0.12

Its authored-runtime licence/provenance is recorded in `ASSET_LICENSES.md`; Android and Windows package gates require it. The static preparation pipeline invokes `validate-dielectric-topology.py` before accepting a thick dielectric. Runtime loading independently rejects open/non-manifold thick meshes so bypassing offline preparation does not silently create invalid transport.

Windows and Android RT Lab expose fixture visibility, transmission, IOR, and roughness. Development checkpoints are `glass-transport` (109, no-fire/settled torch with distant skylight) and `glass-fire-transport` (110, live fire), using the same camera and fixture placement for direct comparison.

## Shader size and cliff investigation

All committed shader changes were compiled with the established `compile-raygen.ps1` `-Os` route and checked against the embedded word stream.

| Boundary | SPIR-V bytes | Instructions | Branch ops | Loops | Selection merges |
| --- | ---: | ---: | ---: | ---: | ---: |
| Task 5 baseline | 494,096 | 27,665 | 3,937 | 58 | 1,581 |
| shared dielectric/terminal helpers (`d13f656`) | 494,160 | 27,667 | 3,937 | 58 | 1,581 |
| material import (`2670d61`) | 494,160 | 27,667 | 3,937 | 58 | 1,581 |
| iterative transport (`57e6833`) | 636,680 | 35,659 | 5,152 | 76 | 2,066 |
| first naive shadow form, investigation checkpoint | 705,508 | 39,390 | 5,752 | 76 | 2,346 |
| batched bounded shadow (`e361a5c`) | 652,592 | 36,434 | 5,226 | 68 | 2,114 |
| final fixture/diagnostics (`e5e7592`) | 653,828 | 36,485 | 5,226 | 68 | 2,114 |

The naive shadow cliff was attributed to optimizer inlining at four duplicated ray-query call sites. The resolved source had one helper definition and no duplicate include; optimized SPIR-V had one `OpFunction`, no `OpFunctionCall`, and 43 static ray-query initialize/proceed/confirm operations. A two-sample batching hypothesis reduced this to 35 static operations and 678,332 bytes. The retained four-sample bounded batch has the same two local-light and two skylight physical shadow samples and the same 4/8 per-query interface budgets, but only 29 static ray-query operations.

Relative to the naive form, the retained final shader is 51,680 bytes smaller (-7.33%), with 2,905 fewer instructions, 526 fewer branch operations, eight fewer loops, and 232 fewer selection merges. Relative to pre-shadow transport it adds 17,148 bytes (+2.69%) and 826 instructions (+2.32%). An unoptimized glslang experiment produced 201,732 bytes / 11,931 instructions by retaining 54 functions and 173 calls, but changing the established compile policy without phone evidence was judged too broad and was rejected.

Final exact shader identity:

- SPIR-V SHA-256: `05a8397ef2361d353632702afc88308f83d862c92ed7c07442bc515871a6ab1a`
- resolved dependency SHA-256: `9b24d63a4c68f86f500aa3214b125708910f72d61e3998cb8d547a4318173c5a`
- embedded include SHA-256: `a77a4e0694faed465363af05dcfb7f5d9f1b710643d83bdafcc49c39d4a6c02e`
- embedded words match: true

Numerical register pressure is unavailable because NVIDIA Nsight is not installed. SPIR-V structure and exact Windows GPU timing are reported without substituting them for phone occupancy evidence.

## Windows captures and water identity

Exact implementation-commit captures use `NVIDIA GeForce RTX 5050 Laptop GPU`, 960x540 dispatch/swapchain, render scale 1.0, honestly presented RT frames, normalized `outputRedBlueSwap`, and 12 timing samples.

- No-fire/skylight checkpoint 109: SHA-256 `f820de64c0f5f023bde4c1a0b94252640b5923c29ef50bb6063b7d048915d98e`; median 6.053150 ms; GPU RT-command-buffer average 0.868518 ms; diagnostics transport 105 / shadow 0.
- Fire-on checkpoint 110: SHA-256 `baa6e170a7e6f7fecfa0cb18663c2b928d2856a5170fd40dc46be1984a462e19`; median 6.067000 ms; GPU RT-command-buffer average 1.089850 ms; diagnostics transport 97 / shadow 0.

The captures show the closed volume's direct through-view, distant bright color/skylight targets, bounded edge/Fresnel response, live warm fire, and wet-stone reflection context. The nonzero transport diagnostics are deliberate deterministic evidence that edge/TIR rays which consume the complete dielectric budget use the bounded fallback; shadow traversal stays within budget.

All 13 authored fixture-hidden Windows PNGs from exact implementation commit `e5e7592` are SHA-256 bit-identical to the reviewed Task 5 baseline in `run-20260826-213806`. The current aggregate diagnostic count is 0/0. The baseline overall median was 6.050300 ms and current was 6.051550 ms (+0.0207%), so the shared refactor introduced neither a capture-byte change nor a material no-op cost cliff on this Windows route.

Exact capture evidence is under `reports/foundation-runs/run-20260826-232002/captures/`.

## Fresh full host/build gate

`tools/run-foundation-validation.ps1 -Mode Host` passed at exact implementation commit `e5e7592a42e92ad8b14474580e534c8facf4f999`:

- evidence root: `reports/foundation-runs/run-20260826-232002`
- shader freshness and tracked-include negative gates: pass
- Windows Debug: 26/26 CTests, 96.74 s
- Windows Release: 26/26 CTests, 46.74 s
- fixed authored Windows captures: 13/13, honest RT presentation
- Android: clean Debug plus unsigned Release plus Release lint; `BUILD SUCCESSFUL in 2m 48s`, 100 actionable tasks (98 executed, two up-to-date)
- Windows/Android package and licence gates: pass, including exact dielectric fixture
- evidence hashing: pass
- repository status before and after the gate: clean

After adding this report, final-head verification reran shader freshness successfully and reran the seven focused dielectric/material/ABI/topology/fixture-slot tests: Debug 7/7 in 3.22 s and Release 7/7 in 2.93 s.

The generated Android validation APK is deliberately unsigned and unpublishable:

- `reports/foundation-runs/run-20260826-232002/artifacts/Horde-Lantern-RT-validation-20260826-232002-Android-UNSIGNED-DO-NOT-PUBLISH.apk`
- SHA-256 `c5e95af72959ec018aaffc43ff91f15e1ccab2f442e50b19e51f27258bff4bf2`

## Device and owner boundaries

Final ADB command: `C:\Users\sam_s\AppData\Local\Android\Sdk\platform-tools\adb.exe devices -l`.

Result: the device list was empty. There was no authorized target to resolve, install to, or byte-compare. The expected prior device is model code `SM-S948B`, but Task 6 records no new phone identity, install, capability, timing, thermal, GPU-power, resource, capture, or lifecycle evidence. Those gates remain open for Task 9 and must use the exact candidate artifact that is ultimately tested.

Automated image inspection can confirm capture structure and deterministic output but cannot establish hands-on perceived glass quality. Owner evaluation of edge readability, distortion/roughness feel, and direct through-view remains an explicit subjective boundary.

Task 5's two-cut combo owner audio/haptic replay remains independently open; Task 6 did not touch that route.

Audio/haptic manual revalidation required: NO — this milestone changes material transport and shader visibility only; audio/haptic state, event timing, playback, spatialisation, and feedback semantics are unchanged.
