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


## Fix Round 1 — deterministic ordering, energy, thin defaults, winding, and exact identity

Date: 2026-08-27 (Australia/Sydney)

This section supersedes any contrary identity, shader, ABI, counter, or final-gate statement earlier in this report. In particular, the first-round image comparator allowed a maximum one-channel difference of one and therefore did **not** prove bit-exact output. A strict SHA-256 audit exposed sparse rounding changes in two fixture-hidden images. Fix Round 1 restores literal byte identity with a separately compiled legacy-inactive shader variant and proves all 13 images by exact hash.

Final implementation head before this report update is `c5527a9d5ec90fcc97248472636bb5cb6dc545fd`; the worktree was clean before and after the full validation run.

### Fix commits

- `13db9b9` — `test: define ordered dielectric energy reference`
- `422df8c` — `fix: honor gltf thin transmission defaults`
- `11a2028` — `fix: validate dielectric winding after transforms`
- `8b44980` — `fix: order and conserve bounded glass transport`
- `59f5cae` — `test: add tinted and millimetre glass proofs`
- `2df21a9` — `fix: preserve exact fixture-hidden transport`
- `c5527a9` — `test: prove grazing and millimetre glass bounds`

### Six Important findings — RED/GREEN resolution

1. **Transparent-shadow candidate order.** The host reference test permutes all 120 orders of five nested/thin interfaces and requires identical RGB output, nesting state, interface count, and overflow state. Runtime shadow transport now repeats a nearest committed `rayQueryEXT` from an advanced origin, with actual accumulated distances and stable progress rules. It never pairs implementation-defined candidate order. Mobile remains four interfaces; High remains eight.
2. **RGB generic shadow transmittance.** Transmittance stays `vec3` through ordinary local, skylight, and selected-fire lighting. Tinted glass therefore filters fire instead of falling back to binary visibility. Behavioral host tests prove channel-separated Beer-Lambert output and prove both an opaque blocker and a high-metallic transmissive material terminate to zero. The shader keeps exactly one/two local-plus-sky samples and the established fire sample counts, while Task 4 reflection-energy ownership remains unchanged.
3. **Energy conservation.** Host and GLSL share one effective Fresnel, including the bounded roughness treatment, then partition reflection and transmission from that value. The exhaustive host sweep covers 499,849 IOR/cosine/roughness combinations and requires finite normalized Fresnel and reflection plus transmission no greater than one. Dedicated IOR 1.5 / roughness 1 and NaN/infinite/negative clamping cases pass.
4. **Thin closed glass and unclosed fallback.** The fixed 4 mm advance / 2 mm `tMin` was replaced by a finite world-scale, hit-distance-aware, normal-aware epsilon. In the authored corridor it is 0.02–0.10 mm, keeps two advances below 1 mm, and remains capped at 0.25 mm for very large finite coordinates. A real 1 mm closed fixture passes direct through-view with zero unclosed-volume diagnostics. If an ordinary terminal is reached with a nonempty closed-volume stack, the path no longer shades through it: it selects the deterministic bounded fallback and increments the dedicated unclosed counter. The exact-zero Snell discriminant is accepted as tangent transmission (`discriminant < 0`, not `<= 0`) and has a boundary test.
5. **glTF defaults.** `KHR_materials_transmission` without `KHR_materials_volume`, and `KHR_materials_volume` with zero thickness, deterministically map to audited `ThinWall`. A contradictory `thinWall=false` override cannot turn zero-thickness glTF material into a thick volume. An explicit positive thickness override selects real closed-volume intent and invokes topology validation. Runtime and offline fixture tests cover extension-only, zero-thickness, override, and positive-thickness cases.
6. **Topology and winding.** Runtime and offline validators require exactly two uses of each shared edge, one in each direction, plus outward signed-volume orientation after baked positive-determinant transforms. Both routes accept the closed fixture and a valid transformed fixture, and reject open, non-manifold, inward-wound, single-face-flipped, and negative-determinant node-transform fixtures with repair instructions. Negative scale is deliberately rejected; the diagnostic requires baking the reflection and reversing triangle winding/normals before import.

The diagnostic ABI remains one fixed 16-byte `RtDielectricDiagnostics` `uvec4` at append-only binding 22, now with distinct offsets for transport overflow, shadow overflow, secondary-dielectric rejection, and terminal-with-open-volume rejection. No existing binding, material stride, descriptor capacity, TLAS capacity, or sample budget changed.

### Inactive-material identity and push ABI

The sparse one-channel differences were caused by compiling the new RGB path behind a dynamic activity branch: even when false at runtime, the compiler changed legacy floating-point instruction formation. Restoring the literal legacy operations inside that dynamic shader was insufficient. The retained solution compiles two raygen pipelines from the same source:

- generic variant: full bounded RGB dielectric transport;
- legacy-inactive variant: `genericTransmissionActive` is a compile-time false constant, preserving the reviewed Task 5 arithmetic.

The CPU derives activity by scanning active `Transmissive` instance metadata and imported transmission material flags/factor/metallic state. It does not branch on the fixture, lantern, or water object. Fixture-hidden authored routes select the legacy pipeline/SBT; any active generic dielectric selects the generic pipeline/SBT. Water retains its established special geometry/flow identity and does not spuriously activate the generic route.

The push ABI appends `genericTransmissionActive` at offset 120 and is now 124 bytes. Compile-time assertions require `sizeof(ScenePushConstants) == 124` and `<= 128`; runtime initialization queries `VkPhysicalDeviceProperties::limits.maxPushConstantsSize` and rejects a device below 124 with an actionable diagnostic. Vulkan's guaranteed minimum/project ceiling remains 128 bytes. The exact Windows RTX device reports 256 bytes. The phone property could not be queried because ADB had no device; that remains part of the exact-device gate rather than being inferred.

Both pipelines use the same three shader groups. On the RTX device the 32-byte handle, 32-byte handle alignment, and 64-byte base alignment produce a 192-byte SBT per pipeline, so the second SBT adds 192 bytes. The extra persistent embedded legacy SPIR-V is 774,584 bytes. Driver-managed pipeline memory is not exposed numerically; there is one additional `VkPipeline`, while shader modules are destroyed after creation. No descriptor or fixed scene-buffer capacity increased.

### Final shader metrics and cliff investigation

| Variant/boundary | Bytes | Instructions | Branch ops | Loops | Selections | Functions/calls | Static query sites |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| reviewed Task 6 before Fix Round 1 | 653,828 | 36,485 | 5,226 | 68 | 2,114 | 1 / 0 | 29 |
| Fix Round 1 before identity split | 771,848 | 42,506 | 6,092 | 89 | 2,499 | 1 / 0 | 29 |
| final generic | 785,676 | 43,378 | 6,263 | 89 | 2,556 | 1 / 0 | 29 |
| final legacy-inactive | 774,584 | 42,711 | 6,155 | 89 | 2,520 | 1 / 0 | 29 |

The Fix Round 1 increase is attributed to deterministic nearest-interface RGB traversal being fully inlined into 21 lighting call sites, plus terminal-open-volume handling and the activity route. Resolved sources contain no duplicate include and there is no accidental loop unrolling; final binaries still contain 29 static ray-query sites. Three optimizer hypotheses were tested without lowering interface budgets or sample/visual quality:

- whole-function-preserving optimization: 188,844 bytes / 11,112 instructions / 568 branches / 10 loops / 241 selections / two static query sites;
- `DontInline` limited to shadow traversal: 628,812 bytes / 35,004 instructions / 4,986 branches / 45 loops / 2,046 selections / nine static query sites;
- the established fully inlined route above.

Both call-preserving variants caused the NVIDIA capture process to hang without presenting for at least five minutes. They were rejected. The final fully inlined binaries complete real RTX `vkCmdTraceRaysKHR` captures. This is a driver-safety choice, not a physical-budget reduction.

Final generic identity: source dependency SHA-256 `8a151c061b31d906568aa79e6f6ba07a16b397a4d70d0406cc673f642efdff4b`; embedded include SHA-256 `3ddaa87b59df1543309d1158645ba553e78778b4cd352ff371e56947865786f8`; SPIR-V SHA-256 `f358351e815f7e0394691f6893e27c2ba37f0a4b1a85f7b60d68a373dd467df1`.

Final legacy identity: source dependency SHA-256 `5f10cd78513710a701a320df0d8a3e201bc7c0de5ec9d80b297498634572c09b`; embedded include SHA-256 `4c9864d665a0fcace4beb58f4823a67395a5f3ba79a468e822e8351be4ebd61e`; SPIR-V SHA-256 `d4ad85bed4bf04a0963d718386d17b7ac86e0bbbbccc1450c8511eaab976ce58`.

Numerical register pressure remains unavailable because NVIDIA Nsight is not installed; shader size/structure and real capture timings are reported without claiming phone occupancy.

### Strict fixture-hidden capture identity

The independent check compares each PNG's literal SHA-256, not a pixel tolerance. Final run `run-20260827-013127` matches reviewed Task 5 run `run-20260826-213806` exactly, 13/13, and all four dielectric counters are zero:

| Capture | Exact SHA-256 |
| --- | --- |
| `00-opening.png` | `fab96a76afa942cc96561cf6dc774e2167706835ac28e0f5b32f460f4585d82b` |
| `01-skeleton.png` | `27646f0f4d4277128637132f9bdf6c80b95c502d84c26423872e40313a9e4ee6` |
| `02-worst-bend.png` | `acd376b717f97d0e4922369aca7b32e20bda5abfb1155f9e8a9151a68e759f1b` |
| `03-lantern-drop.png` | `126a05dc801aae00df61d69c8991b47d73b1601b245f401384d7ff2429ac1c79` |
| `04-skylight.png` | `11fb28bf6de4c858d68fd6df4eb34b2ea1b382789db9c11623e0262926b4d3db` |
| `05-yellow.png` | `b874493f452ebe26acb78575106d439bfbb6108112fc15317dbf7c9b0749b044` |
| `06-blue.png` | `540669c427c34574b6a92624550f166041b1fa138e558c25c687e008ae600558` |
| `07-red.png` | `b17c2bb2dc878b5b74a2898932262d195c170cff33034062c40b95d1d03eec82` |
| `08-green.png` | `51813792e984f1ca627c5002fda3e285427ba49bd8d3a3ab10a7c4de469b41e0` |
| `09-mirror.png` | `0384fbb5fdad251631a96f5c98923ed65f9e154be8aee893bc27b30bf864983e` |
| `10-lich.png` | `6337ef821d787c1299d1c096c13a466d00af09d90682c3558695124400fc666f` |
| `11-finale-roof.png` | `feb8ec4e715c1fe40bde71f90a6b38ad0e97a8be920fc828b7720a78a09ef210` |
| `12-two-enemy-combat.png` | `cac5a343933cddb035f2037d15d0412774347e8948583164c35f634fdd118577` |

Final hidden-run aggregate timing is median 6.053300 ms with GPU RT-command-buffer average 1.023149 ms over 156/155 samples respectively. These RTX timings do not substitute for the missing phone gate.

### Generic glass captures and exact diagnostics

All captures are 960x540, render scale 1.0, honestly presented RT frames on `NVIDIA GeForce RTX 5050 Laptop GPU`, with 12 timing samples and normalized `outputRedBlueSwap`.

| Checkpoint | PNG SHA-256 | Median ms | GPU RT avg ms | Transport overflow | Shadow overflow | Secondary reject | Unclosed volume |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| no-fire / skylight `glass-transport` | `89f3d9956711ab33291b42b453ba50ae8ba3bb03107f8d797bbce97ac6f12401` | 6.060200 | 0.991031 | 106 | 0 | 11 | 36 |
| fire-on `glass-fire-transport` | `3594ad3ab7e091e87771e66e8e4fd81e5d456748fe328427c4980560838abbc2` | 6.062900 | 1.250065 | 94 | 0 | 10 | 53 |
| tinted/fire `glass-tinted-transport` | `722f76f445c17013e15be9688840213a886d8462375d13324c1ac0ff4407b0e8` | 6.050850 | 1.249484 | 94 | 0 | 10 | 53 |
| 1 mm closed `glass-millimetre-closed` | `89371829197fed0b73c26757d4e20cb2b9e25d047b7cc5ef7788b3b0618e1a79` | 6.056450 | 0.973440 | 0 | 0 | 32 | 0 |
| grazing `glass-edge-fresnel` | `3d44dc40efa662009473265ba7567da77c651a60bbf455c73371a3b496df4526` | 6.067700 | 0.875683 | 0 | 0 | 0 | 500 |

Every nonzero counter has a distinct meaning:

- **Transport overflow** means TIR/roughness/internal-edge continuation consumed the fixed 4/8 interface budget and selected the deterministic bounded fallback. It is 106 for the settled standard view and 94 for the fire/tinted views; it is zero for the 1 mm direct-view and edge checkpoints.
- **Shadow overflow** is zero in every checkpoint, proving sampled local/skylight/fire shadow paths stayed within their fixed 4/8 interface ceilings.
- **Secondary rejection** means the single allowed terminal reflection query hit the same generic dielectric and was rejected instead of recursively evaluating it. Counts are 11 standard, 10 fire/tinted, 32 for the 1 mm view, and zero edge.
- **Unclosed volume** means a refracted path reached an ordinary terminal with its closed-volume stack still nonempty and was deterministically rejected. Temporary instrumentation first proved all 38 probed events were ordinary-terminal-with-open-stack, not no-hit. A second route-unattributed probe recorded first-interface incidence: maximum cosine 0.185004 and summed cosine 0.900359 over 38 events, mean 0.023694. That retained probe established grazing incidence for its sampled aggregate, but it could not distinguish primary from shadow traversal and therefore does not support attributing every aggregate unclosed event to the primary route. The final standard aggregate count is 36; camera/fire sampling changes make fire and tinted 53; the deliberately grazing edge checkpoint is 500. The same closed fixture scaled to exactly 1 mm has zero unclosed events in direct through-view. Temporary probe encoding was removed; Fix Round 2 below records persistent route-specific counters.

Reducing the normal bias to one eighth increased the standard probe from 36 to 38 unclosed events, so that hypothesis was rejected and the full bounded normal-aware bias retained. The corrected edge checkpoint yaw is -0.65 radians; its fixture is actually in view and intentionally exercises silhouette/Fresnel behavior rather than looking away.

Automated capture inspection verifies route, structure, hashes, counters, and timing only. Hands-on perceived glass quality—edge readability, refraction/roughness feel, and direct through-view—is still an owner boundary.

### Final focused and full validation

Focused final-head CTests from the fresh build tree passed:

- Debug: 8/8 in 18.91 s;
- Release: 8/8 in 18.71 s;
- coverage: dielectric math, development fixture/checkpoints, scene ABI, static GLB runtime validation, ABI generator, compiler strategy, offline topology tool, and character/render-slot integration.

Fresh full run `reports/foundation-runs/run-20260827-013127` passed at exact commit `c5527a9d5ec90fcc97248472636bb5cb6dc545fd`:

- shader freshness for generic and legacy variants: pass;
- shader/ABI/topology negative safety gates: pass;
- fresh Windows Debug: 27/27, 104.43 s;
- fresh Windows Release: 27/27, 56.59 s;
- deterministic Windows captures: 13/13 honestly presented; independent strict hash proof above;
- Android: clean Debug, unsigned Release, and Release lint; `BUILD SUCCESSFUL in 2m 53s`, 100 actionable tasks (98 executed, two up-to-date);
- Windows/Android package and licence gate: pass;
- evidence hashing: pass;
- status before and after the run: clean.

The generated Android validation APK is deliberately unsigned and unpublishable:

- `reports/foundation-runs/run-20260827-013127/artifacts/Horde-Lantern-RT-validation-20260827-013127-Android-UNSIGNED-DO-NOT-PUBLISH.apk`
- 74,388,092 bytes
- SHA-256 `4bf28a35f68b01198c326011ac95888db51bf22f0d5276b8d6a40c2d2eb4ed99`

Final `adb devices -l` again returned an empty device list. No exact artifact was installed and no phone parity, matched Mobile/High 75%, separate 100%, warmed timing/thermal/GPU-power/resources, or Home/resume evidence is claimed. Exact `SM-S948B` Task 6 device acceptance remains a hard Task 9 gate.

Task 5's two-cut combo owner audio/haptic replay remains separately open and was not changed by this fix round.

Audio/haptic manual revalidation required: NO — this milestone changes material transport and shader visibility only; audio/haptic state, event timing, playback, spatialisation, and feedback semantics are unchanged.

## Fix Round 2 — component topology parity and route-specific diagnostics

Date: 2026-08-27 (Australia/Sydney)

This section supersedes the earlier thick-topology scope, diagnostic-record size, counter table, shader identity, and final-gate details. The implementation head before this report update is `b994205d2f6513d5e5a8db3235ead1ffbf7a0f65`.

### Fix commits

- `9f7fa15` — `test: define dielectric component topology parity`
- `b961217` — `fix: validate dielectric topology by component`
- `aadc9b1` — `test: define split unclosed diagnostics`
- `b994205` — `fix: split persistent unclosed diagnostics`

The topology RED tests showed two distinct defects: the old offline route rejected a valid closed shell split across primitives/nodes as eight boundary edges, while the old runtime route accepted a material containing one large outward shell and one smaller inward shell because their signed volumes were summed at material scope. The diagnostic RED tests also proved the generated ABI and both platform manifests had only the aggregate terminal-open-volume count.

### Component-aware runtime/offline topology contract

Runtime and offline preparation now aggregate every baked triangle sharing a real thick dielectric material before validating topology. They weld equivalent baked positions across primitive-local and node-local vertex identities using the same deterministic material-bounds-relative quantized key. The documented tolerance is `clamp(max material extent * 1e-6, 1e-7 m, 1e-5 m)`: 0.1–10 micrometres. Its origin is the material's baked bounds minimum, so the key is deterministic and scale-aware. This closes transform-rounding seams without merging the deliberately separate shells in the 50-micrometre-gap fixture.

Triangles are partitioned into edge-connected components after welding. Every component independently requires each undirected edge exactly twice, once in each direction, and a finite positive signed volume. Diagnostics identify material, component number, and contributing node/primitive sources, then prescribe closing/welding, repairing winding, baking a reflected transform with reversed winding/normals, or using audited `thinWall=true` only for an intentional pane. Negative-determinant transforms remain an actionable rejection before baked topology acceptance.

Runtime/offline parity uses the same tracked fixtures and now proves:

- a valid closed shell split across two nodes/primitives passes;
- two disconnected outward shells sharing one material pass, including the close-separation weld guard;
- one large outward shell plus a smaller inward shell sharing one material fails on component 2;
- one component with a single flipped face fails;
- valid baked positive transforms pass, while inward winding, open/non-manifold topology, and negative-determinant transforms fail.

### Append-only diagnostic ABI

Descriptor binding 22 and all original offsets retain their meanings. `RtDielectricDiagnostics` expands from 16 to 32 bytes at 16-byte alignment:

| Offset | Field | Meaning |
| ---: | --- | --- |
| 0 | `transportOverflowCount` | primary bounded transport exhausted its interface budget |
| 4 | `shadowOverflowCount` | transparent-shadow traversal exhausted its interface budget |
| 8 | `secondaryDielectricRejectCount` | the one terminal reflection hit a dielectric and was rejected rather than recursed |
| 12 | `unclosedVolumeCount` | backward-compatible total terminal-with-open-volume count |
| 16 | `primaryUnclosedVolumeCount` | total subset emitted by primary dielectric transport |
| 20 | `shadowUnclosedVolumeCount` | total subset emitted by transparent-shadow traversal |
| 24, 28 | reserved | zeroed append-only capacity |

Every terminal-open-volume rejection increments the legacy total and exactly one route-specific counter. Generated CPU/GLSL ABI, allocation, clear, readback, move/reset state, Windows nested capture manifests, and Android flat debug state all use the expanded record. The push block remains 124 bytes; `vulkaninfoSDK` still reports `maxPushConstantsSize = 256` on the exact Windows device, above the 124-byte requirement and 128-byte project ceiling.

### Shader re-attribution

The two new atomic route counters add 3,552 bytes and 136 instructions to each fully inlined variant. Branches, loops, selections, query-site counts, sample counts, physical budgets, and dual-pipeline selection are unchanged from Fix Round 1.

| Variant | Bytes | Instructions | Branch ops | Loops | Selections | Functions/calls | Static query sites |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| generic | 789,228 | 43,514 | 6,263 | 89 | 2,556 | 1 / 0 | 29 |
| legacy-inactive | 778,136 | 42,847 | 6,155 | 89 | 2,520 | 1 / 0 | 29 |

Generic identity: dependency SHA-256 `0ea1b4af94d90eb9efce2359280d9506ef22598e97f5e7e87a4e28c8a35fc84d`; embedded include SHA-256 `406bcec3524f7b6adf0280de218718b478d9505eabf499f28bf449e43a797236`; SPIR-V SHA-256 `f29f2f537883d9e5deb5d56ad74a1cf35b3e88ed16f773e26e8c5092c704f594`.

Legacy-inactive identity: dependency SHA-256 `71d07d1670d820efb65566a7b2e38031361cec19e9ad3f4e96b8bdf4c993363b`; embedded include SHA-256 `4be7cfd27a83b695b5b1a7e3767ad1982094a020d5e661135d9f19e02e0a1644`; SPIR-V SHA-256 `ac99c30e6f857593df534567a4b5c9efadddc6d8ff92c47b7e033d836edc6ed4`.

Both remain driver-safe, fully inlined, and complete real RTX `vkCmdTraceRaysKHR` captures. There is no new shader cliff: the added diagnostics did not duplicate includes/helpers or change loop/query structure.

### Route-split glass checkpoint evidence

All checkpoints are 960x540 at render scale 1.0, honestly presented on `NVIDIA GeForce RTX 5050 Laptop GPU`, with normalized `outputRedBlueSwap`. Counter order below is deliberately explicit: transport overflow, shadow overflow, secondary dielectric reject, legacy total unclosed, primary unclosed, shadow unclosed.

| Checkpoint | PNG SHA-256 | Median ms | GPU RT avg ms | Transport | Shadow overflow | Secondary | Total unclosed | Primary unclosed | Shadow unclosed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| no-fire/skylight `glass-transport`, warmed | `89f3d9956711ab33291b42b453ba50ae8ba3bb03107f8d797bbce97ac6f12401` | 6.047950 | 0.984268 | 106 | 0 | 11 | 36 | 3 | 33 |
| fire-on `glass-fire-transport` | `3594ad3ab7e091e87771e66e8e4fd81e5d456748fe328427c4980560838abbc2` | 6.089800 | 1.290121 | 94 | 0 | 10 | 53 | 2 | 51 |
| tinted/fire `glass-tinted-transport` | `722f76f445c17013e15be9688840213a886d8462375d13324c1ac0ff4407b0e8` | 6.064550 | 1.240553 | 94 | 0 | 10 | 53 | 2 | 51 |
| 1 mm closed `glass-millimetre-closed` | `89371829197fed0b73c26757d4e20cb2b9e25d047b7cc5ef7788b3b0618e1a79` | 6.060700 | 0.953594 | 0 | 0 | 32 | 0 | 0 | 0 |
| edge/Fresnel `glass-edge-fresnel` | `3d44dc40efa662009473265ba7567da77c651a60bbf455c73371a3b496df4526` | 6.065750 | 0.842464 | 0 | 0 | 0 | 500 | 475 | 25 |

The standard closed fixture's aggregate 36 is therefore not a primary-only grazing result: three rejections came from primary transport and 33 from transparent-shadow traversal. Fire and tinted each split 2 primary / 51 shadow. The deliberate edge checkpoint splits 475 primary / 25 shadow. These counters diagnose bounded route termination; they do not by themselves establish a geometric cause. The earlier temporary aggregate incidence probe had the corrected sum 0.900359 and mean 0.023694, but lacked route identity and is not used to infer primary grazing from shadow counts. The 1 mm closed-volume proof has zero primary and shadow unclosed events; its 32 secondary rejects are the intentional one-terminal-reflection non-recursion rule. Transport overflow remains deterministic bounded fallback, shadow overflow remains zero in every checkpoint, and secondary rejection remains distinct from both interface overflow and terminal-open-volume rejection.

### Exact inactive identity and final gates

Fresh full run `reports/foundation-runs/run-20260827-022655` passed at exact implementation commit `b994205d2f6513d5e5a8db3235ead1ffbf7a0f65`:

- shader freshness for generic/legacy and tracked-include/compiler/package negative gates: pass;
- Windows Debug: 27/27, 112.29 s;
- Windows Release: 27/27, 59.14 s;
- fixed Windows captures: 13/13 honestly presented;
- independent literal SHA-256 comparison against reviewed Task 5 run `run-20260826-213806`: 13/13 exact, with all six dielectric counters zero;
- hidden-run aggregate timing: 6.054200 ms median, GPU RT-command-buffer average 1.374902 ms over 156/155 samples;
- Android clean Debug, unsigned Release, Release lint: `BUILD SUCCESSFUL in 3m 10s`, 100 actionable tasks (98 executed, two up-to-date);
- Windows/Android package and dielectric-fixture licence gate: pass;
- evidence hashing: pass; status before/after the gate: clean.

Final focused coverage passed Debug 8/8 in 18.59 s and Release 8/8 in 18.05 s: dielectric math, development fixture/checkpoints, scene ABI, static GLB/runtime validation, ABI generator, compiler strategy/negative proof, offline topology, and character/render-slot integration.

The exact unsigned, unpublishable Android validation artifact is:

- `reports/foundation-runs/run-20260827-022655/artifacts/Horde-Lantern-RT-validation-20260827-022655-Android-UNSIGNED-DO-NOT-PUBLISH.apk`
- 74,491,708 bytes
- SHA-256 `5b512b0b99236b4b686d187c4396913caaaef80d739f38d2a95531a8443609c8`

Final `adb devices -l` returned an empty device list. No exact artifact was installed and no phone parity, matched Mobile/High 75%, separate 100%, warmed timing/thermal/GPU-power/resources, or Home/resume evidence is claimed. Exact `SM-S948B` Task 6 device acceptance remains a hard Task 9 gate.

Automated capture inspection verifies route, structure, hashes, counters, and timing only. Hands-on perceived glass quality remains an owner boundary. Task 5's two-cut combo owner audio/haptic replay remains separately open and was not changed by this fix round.

Audio/haptic manual revalidation required: NO — this milestone changes material transport and shader visibility only; audio/haptic state, event timing, playback, spatialisation, and feedback semantics are unchanged.

## Fix Round 3 — metre-space spatial-weld parity

Date: 2026-08-27 (Australia/Sydney)

This narrowly scoped round changes only runtime/offline thick-dielectric topology preparation and its tracked fixtures. Shader source, generated ABI, material ABI, descriptor bindings, dual-pipeline selection, physics budgets, captures, and glass diagnostics are unchanged. Implementation head before this report update is `0dd96cca9d67eab4899cad1fc729e1038cdf2ac0`.

### Commits and RED evidence

- `c424b08` — `test: define metre-space dielectric weld parity`
- `0dd96cc` — `fix: weld dielectric topology in metre space`

The shared tracked RED fixtures exposed every reviewed defect against the old validators:

- runtime rejected the sub-tolerance seam whose equivalent vertices lie on opposite rounded-cell sides;
- runtime accepted same-cell diagonal vertices whose Euclidean separation exceeds tolerance;
- reordering the two mixed-orientation nodes changed the inward shell diagnostic from component 2 to component 1;
- offline accepted both zero `metresPerUnit` and `3.5e38`, which overflows the runtime float representation;
- a centimetre-unit node-translation seam passed runtime but failed offline because offline transformed positions and translations remained in source units.

These were behavioral failures from real GLBs and both validators, not source-string assertions.

### Matched metre and weld algorithm

Offline validation now converts and validates `metresPerUnit` with the runtime float domain: it must be finite, representable as a finite float, and greater than zero. Node matrices use runtime-float semantics; the complete transformed point follows `StaticMeshAsset` ordering, multiplying all three linear point terms and the world translation into metres before topology validation. Material bounds, weld tolerance, distance predicates, and signed volumes therefore all operate in metres in both implementations.

Rounded-cell equality is removed from C++ and Python. The matched algorithm is:

1. Sort every triangle-corner occurrence lexicographically by baked metre-space position.
2. Use floor-based spatial buckets whose cell width is the documented scale-aware tolerance `clamp(material extent * 1e-6, 0.1 um, 10 um)`.
3. Search the current cell and all 26 adjacent cells.
4. Accept a weld only when explicit Euclidean squared distance is no greater than tolerance squared.
5. Choose the nearest canonical representative, breaking an exact tie by its stable lexicographic representative id.
6. Put representatives—not subsequently joined members—into buckets. A proximity chain therefore cannot bridge two points that are farther apart than tolerance.
7. Sort edge-connected components by their canonical welded-vertex keys, and sort their triangles canonically before signed-volume accumulation. Component identity no longer depends on node or primitive traversal order.

The coordinate range is checked before conversion to fixed bucket coordinates. Existing actionable material/component/node/primitive diagnostics remain, including open, non-manifold, inconsistent-winding, inward-wound, negative-determinant, and now out-of-range weld-coordinate failures.

### Runtime/offline parity fixtures

The same committed GLBs and manifests drive the C++ runtime loader and Python offline gate:

| Fixture/scale | Expected matched outcome |
| --- | --- |
| centimetre units, about 11 mm material extent, seam across opposite half-cell boundaries | pass, 2 closed components |
| same fixture with nodes/primitives reordered | pass, 2 closed components |
| centimetre units, 40 nm translated seam below the 0.1 um minimum clamp | pass, 2 closed components |
| centimetre units, same rounded cell but diagonal distance greater than 0.1 um | fail, open component 2 |
| ten-metre units, two outward shells with a 50 um separation at the 10 um maximum clamp | pass, 2 closed components; panes remain distinct |
| ten-metre units, translated seam whose metre-space separation exceeds the 10 um clamp | fail, open component 2 |
| reordered large outward plus smaller inward shells | fail, inward component 2 in both routes |
| zero or runtime-float-overflow `metresPerUnit` | reject before topology validation |

The prior split-shell, multiple-shell, open, non-manifold, flipped-face, inward, millimetre, valid-transform, negative-determinant, and thin-wall fixtures remain in the same parity gate.

### Verification and unchanged invariants

Focused final implementation-head tests passed:

- Debug runtime/offline CTests: 2/2 in 3.46 s;
- Release runtime/offline CTests: 2/2 in 3.39 s;
- direct offline fixture suite: pass;
- standalone Debug and Release runtime static-GLB contracts: pass.

Fresh normal Host run `reports/foundation-runs/run-20260827-030354` passed at exact implementation commit `0dd96cca9d67eab4899cad1fc729e1038cdf2ac0`:

- shader freshness and all negative safety gates: pass;
- Windows Debug: 27/27 in 112.52 s;
- Windows Release: 27/27 in 67.07 s;
- Windows captures: 13 honestly presented RT frames;
- independent literal SHA-256 comparison with reviewed Task 5 `run-20260826-213806`: exact 13/13;
- all six hidden-route dielectric counters: zero;
- hidden aggregate timing: 6.053500 ms median, GPU RT-command-buffer average 0.866746 ms over 156/155 samples;
- Android clean Debug, unsigned Release, and Release lint: `BUILD SUCCESSFUL in 3m 18s`, 100 actionable tasks (98 executed, two up-to-date);
- Windows/Android package and licence gate: pass;
- evidence hashing: pass; worktree status before and after the run: clean.

Both raygen artifacts are byte-identical to Fix Round 2. Generic remains 789,228 bytes / 43,514 instructions / SHA-256 `f29f2f537883d9e5deb5d56ad74a1cf35b3e88ed16f773e26e8c5092c704f594`; legacy-inactive remains 778,136 bytes / 42,847 instructions / SHA-256 `ac99c30e6f857593df534567a4b5c9efadddc6d8ff92c47b7e033d836edc6ed4`. Generated ABI and the 124-byte push block did not change.

The exact unsigned, unpublishable Android validation artifact is:

- `reports/foundation-runs/run-20260827-030354/artifacts/Horde-Lantern-RT-validation-20260827-030354-Android-UNSIGNED-DO-NOT-PUBLISH.apk`
- 74,591,164 bytes
- SHA-256 `e7b09f25c07cc008f112df02fa3bea8354d3810002f479fb5c966b58b3764642`

Final `adb devices -l` returned an empty device list. No installation or phone-runtime evidence is claimed; exact `SM-S948B` acceptance remains the existing hard Task 9 gate. Hands-on perceived glass quality remains an owner boundary. Task 5's two-cut combo owner audio/haptic replay remains separately open and unchanged.

Audio/haptic manual revalidation required: NO — this milestone changes material transport and shader visibility only; audio/haptic state, event timing, playback, spatialisation, and feedback semantics are unchanged.


## Fix Round 4 — runtime-float transform and weld-domain safety parity

Date: 2026-08-27 (Australia/Sydney)

This strictly numeric round supersedes Fix Round 3's claims about offline float conversion, transform arithmetic, bucket origin, and coordinate-range safety. It does not change shaders, generated ABI, descriptor bindings, the 124-byte push block, dual-pipeline selection, glass transport, capture routes, or diagnostics.

### Commits and behavioral RED evidence

- `ac59543` — `test: define dielectric numeric parity and safety`
- `6a5ea7e` — `fix: align dielectric numeric domains`
- `0911fde` — `chore: ignore generated Python bytecode`

The shared tracked RED contracts demonstrated the reviewed defects before implementation:

- the old offline bucket rule rejected the exact representable value immediately inside the negative `-2^63` cell boundary;
- the old runtime accepted GLBs whose first representable baked float crossed either positive or negative safe-domain boundary;
- a closed shell split between cgltf-equivalent parent/child TRS and matrix paths failed offline with eight boundary edges because Python regrouped the float expressions;
- positive `1e-50` `metresPerUnit` rounded to float32 zero offline but was not rejected at the manifest boundary;
- runtime NaN/+infinity/-infinity POSITION data reached the later topology bucket path after bounds processing instead of an early finite-domain rejection;
- the runtime NaN manifest classification did not match the audited metres-per-unit rejection.

These are behavioral/reference tests against real GLBs, the runtime loader, the offline CLI, and one exact shared numeric case file; none are source-string assertions.

### Exact runtime-float transform contract

The complete relevant cgltf implementation in `third_party/cgltf/cgltf.h` was read before the offline implementation changed. Offline validation now mirrors it expression-for-expression:

1. JSON translation, rotation, scale, and matrix inputs round to float32 exactly as cgltf storage does.
2. `cgltf_node_transform_local` TRS products, left-associated additions/subtractions, and output scales round after every `cgltf_float` operation; Python does not regroup quaternion terms or use implicit double/FMA arithmetic.
3. `cgltf_node_transform_world` begins with the node-local matrix, applies each parent in ancestor order with the same three-product column expressions, then performs the three separate parent-translation additions.
4. StaticMeshAsset point baking is matched in the original order: each matrix/point product is multiplied by the runtime-float `metresPerUnit`, the world translation is independently multiplied into metres, and the four terms are added left-associatively in float32.
5. `metresPerUnit` itself first converts to exact runtime float32 and is then revalidated as finite and greater than zero. Positive underflow, runtime-float overflow, and NaN therefore have the same manifest classification as runtime.

The large legal transformed fixture uses paired parent and child transforms: one half supplies rotated/scaled/translated TRS, and the other supplies the exact cgltf float32 matrices. Runtime asserts the two baked world matrices are bit-identical. Runtime and offline then both accept the joined shell, and offline reports exactly one canonical closed/manifold component at the 10 micrometre maximum weld clamp.

### Finite exclusive cell-domain contract

Every runtime baked POSITION coordinate is now checked before it contributes to global bounds. Thick-material aggregation defensively repeats the same check before its material bounds. Offline checks transformed thick POSITION data before `min`, `max`, lexicographic ordering, or bucket conversion. Diagnostics name material, node, mesh where available, and primitive, identify the finite deterministic weld domain, and prescribe keeping baked metre-space coordinates strictly inside the boundary.

The shared constants are the 0.1 micrometre minimum tolerance, 10 micrometre maximum tolerance, and the exactly representable hexadecimal-double exclusive limit `0x1p63`. A coordinate is safe only when its minimum-tolerance scaled value is finite and strictly greater than `-2^63` and strictly less than `2^63`. The same exact predicate is repeated immediately before every floor/int64 conversion; it is not derived from a rounded `double(INT64_MAX - 1)` value.

Buckets now use a fixed zero metre-space origin and signed int64 cells. Neighbor traversal guards both `INT64_MIN` and `INT64_MAX` before addition. This grid shift does not change the established geometry rule: all 27 adjacent/current buckets are still searched, welding still requires explicit Euclidean distance no greater than tolerance, only canonical representatives enter buckets, and nearest representative plus stable id remains the tie rule. The prior seam, diagonal, reorder, proximity-chain, 50 micrometre pane, split-shell, multi-shell, winding, and negative-transform fixtures all retain their outcomes.

The shared exact-double file covers `nextafter`, exact, and next-after-outside values at both signed boundaries. The GLB suite additionally covers the nearest runtime-float POSITION immediately inside, the first runtime-float crossing, and an outside value on both signs. Inside shells pass as one component; crossing/outside shells reject before bounds; NaN, positive infinity, and negative infinity reject with the same finite-domain classification.

### Focused safety and parity verification

Final implementation-head focused tests passed:

- Debug runtime plus offline topology CTests: 2/2 in 11.52 s;
- Release runtime plus offline topology CTests: 2/2 in 6.45 s;
- direct offline numeric/reference and full fixture suite: pass;
- exact runtime TRS/matrix world-bit assertion and shared six-case cell boundary file: pass.

MSVC AddressSanitizer was available. A separate Debug ASan build ran the complete static-GLB asset contract, including NaN/+infinity/-infinity POSITION and all six boundary GLBs, with no sanitizer report. The normal Debug configuration also retained its runtime checks; there is no pre-floor invalid float-to-int conversion on either route.

### Shader, capture, Host, package, and device invariants

Fresh normal Host run `reports/foundation-runs/run-20260827-034316` passed at exact commit `0911fde17c147e236cde14a97b68a15d3846a804`:

- shader freshness, compiler strategy, ABI generator, and all negative safety gates: pass;
- Windows Debug: 27/27 in 114.85 s;
- Windows Release: 27/27 in 66.79 s;
- deterministic Windows captures: 13 honestly presented RT frames;
- independent literal SHA-256 comparison with reviewed Task 5 `run-20260826-213806`: exact 13/13, not a channel-tolerance result;
- all six hidden-route dielectric counters: zero;
- hidden aggregate timing: 6.050200 ms median versus 6.050300 ms baseline (`-0.002%`), GPU RT-command-buffer average 1.228823 ms over 155 samples;
- Android clean Debug, unsigned Release, and Release lint: `BUILD SUCCESSFUL in 3m 17s`, 100 actionable tasks (98 executed, two up-to-date);
- Windows/Android package and dielectric-fixture licence gate: pass;
- evidence hashing: pass; worktree status before and after the gate: clean.

Shader source, includes, generated ABI, and embedded artifacts have no diff from the reviewed pre-round head. Fresh measurements remain:

| Variant | Bytes | Instructions | Branch ops | Loops | Selections | Functions/calls | Static query sites | SHA-256 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| generic | 789,228 | 43,514 | 6,263 | 89 | 2,556 | 1 / 0 | 29 | `f29f2f537883d9e5deb5d56ad74a1cf35b3e88ed16f773e26e8c5092c704f594` |
| legacy-inactive | 778,136 | 42,847 | 6,155 | 89 | 2,520 | 1 / 0 | 29 | `ac99c30e6f857593df534567a4b5c9efadddc6d8ff92c47b7e033d836edc6ed4` |

The exact unsigned, unpublishable Android validation artifact is:

- `reports/foundation-runs/run-20260827-034316/artifacts/Horde-Lantern-RT-validation-20260827-034316-Android-UNSIGNED-DO-NOT-PUBLISH.apk`
- 74,611,276 bytes
- SHA-256 `19560fc25cdf751c712fab3a2dc48336cb5189b5342cf5043edef9068e2469c8`

Final `adb devices -l` returned an empty device list. No artifact was installed, and no phone parity, matched Mobile/High 75%, separate 100%, warm timing/thermal/GPU-power/resources, or Home/resume evidence is claimed. Exact `SM-S948B` Task 6 acceptance remains the hard Task 9 gate. Hands-on perceived glass quality remains an owner boundary. Task 5's two-cut combo owner audio/haptic replay remains separately open and unchanged.

Audio/haptic manual revalidation required: NO — this milestone changes material transport and shader visibility only; audio/haptic state, event timing, playback, spatialisation, and feedback semantics are unchanged.


## Fix Round 5 — cross-target non-contracted transform arithmetic

Date: 2026-08-27 (Australia/Sydney)

This final round closes the remaining compiler-semantic parity risk. It changes only build policy, its executable object-code gate, and validation orchestration. Shader source, generated ABI, descriptor bindings, push constants, dual-pipeline selection, glass transport/diagnostics, and fixture geometry are unchanged from `1c91d42`.

### Commits and RED object-code proof

- `520fcc1` — `test: gate strict asset transform arithmetic`
- `6c261d2` — `fix: enforce strict asset transform arithmetic`

The implementation-unit audit found exactly two production translation units:

- `src/scene/assets/GltfDocument.cpp` defines `CGLTF_IMPLEMENTATION`, so it owns the real `cgltf_node_transform_local` and `cgltf_node_transform_world` bodies used by runtime and tests;
- `src/scene/assets/StaticMeshAsset.cpp` owns the local `TransformPoint` baked-position arithmetic and calls the cgltf world transform. `AssetValidation.cpp` calls the out-of-line cgltf implementation but does not implement or inline those transform bodies.

The new executable gate first ran against the prior normal Gradle/CMake ARM64 Debug objects. It failed for the intended behavioral reason, recorded exact compile commands/object hashes, and did not inspect a surrogate:

| Prior object | SHA-256 | Audited contracted instructions |
| --- | --- | --- |
| `GltfDocument.cpp.o` | `8024a6d8b4307b5e7af1efdaef5b71e1adf3e5665985d7cdd6995bb216b37315` | local: 11 (`fmsub`, `fnmsub`, `fmadd`); world: 16 (`fmadd`, `fmla`) |
| `StaticMeshAsset.cpp.o` | `3d4154e2b4ecf2ced0c6efeb008efa2bbf429304927767d623635e8f68152d44` | `TransformPoint`: 4 (`fmadd`, `fmla`) |

Both prior compile commands also lacked `-fno-fast-math` and `-ffp-contract=off`; the local quaternion transform had zero separate subtract instructions. This is the retained RED evidence for the Android contraction mismatch.

### Narrow compiler policy and executable gate

`cmake/HordeRtStrictAssetMath.cmake` applies a source property to those two absolute production sources in both the root and Android CMake directories. Therefore it covers the renderer library and every root test target that recompiles either source, while leaving unrelated renderer hot paths alone:

- MSVC: `/fp:strict`;
- Android Clang, other Clang, AppleClang, and GCC: `-fno-fast-math -ffp-contract=off`;
- an unrecognized compiler fails CMake configuration instead of silently using an unaudited floating-point contract.

`tests/AssetMathCompilerContractTests.ps1` builds the real `arm64-v8a` object through the normal Gradle/CMake native task unless explicitly reusing a just-clean-built artifact. It selects the actual compile database, resolves the exact object paths from each compile command, uses the NDK's sibling `llvm-objdump`, and isolates the exact demangled symbol ranges. It rejects `fmadd`, `fmsub`, `fnmadd`, `fnmsub`, `fmla`, and `fmls` families, requires separate multiply/add operations and a separate subtract in the local quaternion transform, validates the strict flags, and records compiler version, full compile commands, object SHA-256 values, disassembly, per-symbol counts, and failures in JSON. The fresh Host gate runs this audit for both Android Debug and Release/RelWithDebInfo after the all-ABI clean build.

Fresh NDK r26.1.10909125 / Android Clang 17.0.2 evidence is in `reports/foundation-runs/run-20260827-042208/asset-math-compiler-contract`:

| ARM64 configuration/object | SHA-256 |
| --- | --- |
| Debug `GltfDocument.cpp.o` | `5db20c11b847486b1ee5de8ae7df55781e0435733021e86fd4f39ea3c5a36f69` |
| Debug `StaticMeshAsset.cpp.o` | `017438289fdceffd649ecc5c8d847a221ff3814a7858cfdc9851fb875569c4a6` |
| RelWithDebInfo `GltfDocument.cpp.o` | `fb3aaaf4b5f89b9a456de73f396b9e4b0f56ab85bf6ffba07511576a5a2bdc34` |
| RelWithDebInfo `StaticMeshAsset.cpp.o` | `f9bc43e913accbb53aba74caed39d76349c7efaded874389c8bec6c439bc4560` |

Debug and RelWithDebInfo produce the same audited structure: local 62 instructions with 18 separate multiplies, six adds, eight subtracts; world 94 with 24 multiplies and 18 adds; `TransformPoint` 30 with 12 multiplies and six adds. Every contracted-instruction list is empty. All eight actual Android compile databases (four ABIs times Debug/RelWithDebInfo) contain both required flags on both audited sources, no fast-math enablement, and present built objects.

Fresh MSVC 19.44.35227 TLogs from `horde_rt_probe_core`, `horde_rt_held_item_socket_tests`, and `horde_rt_static_gltf_asset_tests` record `/fp:strict` for both audited sources in Debug and Release. Exact-range COFF disassembly also contains zero contracted instructions. Debug local/world/point separate multiply-add-subtract counts are `45/3/9`, `9/9/0`, and `18/9/0`; Release counts are `45/3/9`, `36/27/0`, and `18/9/0`.

### Behavioral parity, safety, and full validation

The existing real strict C++ static-GLB test and offline validator both retain the complete Fix Round 4 suite. In particular, the large legal rotated high-scale TRS and equivalent matrix paths still produce bit-identical runtime world matrices and the same accepted single canonical component offline. The minimum/maximum clamp seams, 50 micrometre disconnected-pane guard, reordered nodes/primitives, float underflow/overflow/NaN manifests, NaN/+infinity/-infinity POSITION data, and six exclusive cell-boundary cases retain matched outcomes.

Focused implementation-head verification:

- Debug runtime plus offline topology: 2/2 passed in 11.74 s;
- Release runtime plus offline topology: 2/2 passed in 6.39 s;
- ARM64 Debug and Release/RelWithDebInfo object-code gates: pass;
- MSVC AddressSanitizer Debug static-GLB malformed-coordinate suite: pass in 0.62 s after exposing the installed VS ASan runtime DLL on `PATH`; no sanitizer report;
- QEMU `qemu-aarch64` and `qemu-aarch64-static`: not installed, so no emulator was installed or used.

Fresh normal Host run `reports/foundation-runs/run-20260827-042208` passed at exact implementation commit `6c261d272130539e349ea296b413db5a6cfbbe24`:

- shader freshness, fully inlined compiler strategy, generated ABI, topology, tracked-include, packaging, and negative safety gates: pass;
- Windows Debug: 27/27 in 108.50 s;
- Windows Release: 27/27 in 60.86 s;
- deterministic Windows captures: 13 honestly presented RT frames;
- independent literal SHA-256 comparison with reviewed Task 5 `run-20260826-213806`: exact 13/13, with maximum channel difference zero at every checkpoint;
- hidden-route dielectric counters in unambiguous record order: transport overflow 0, shadow overflow 0, secondary dielectric rejection 0, aggregate unclosed volume 0, primary unclosed volume 0, shadow unclosed volume 0;
- hidden aggregate timing: 6.051600 ms median; GPU RT-command-buffer average 1.160157 ms over 155 samples; versus the Task 5 matched median 6.050300 ms, `+0.021%`;
- Android all four ABIs, clean Debug, unsigned Release, and Release lint: `BUILD SUCCESSFUL in 5m 02s`, 100 actionable tasks (98 executed, two up-to-date);
- both post-clean ARM64 disassembly/flag gates, validation package/licence gate, and evidence hashing: pass; worktree status before and after the run: clean.

Shader source, includes, generated ABI, and embedded artifacts have no diff from `1c91d42`. The unchanged generic artifact remains 789,228 bytes / 43,514 instructions / 6,263 branch operations / 89 loops / SHA-256 `f29f2f537883d9e5deb5d56ad74a1cf35b3e88ed16f773e26e8c5092c704f594`. The unchanged legacy-inactive artifact remains 778,136 bytes / 42,847 instructions / 6,155 branch operations / 89 loops / SHA-256 `ac99c30e6f857593df534567a4b5c9efadddc6d8ff92c47b7e033d836edc6ed4`.

The exact unsigned, unpublishable Android validation artifact is:

- `reports/foundation-runs/run-20260827-042208/artifacts/Horde-Lantern-RT-validation-20260827-042208-Android-UNSIGNED-DO-NOT-PUBLISH.apk`
- 74,611,228 bytes
- SHA-256 `95b4070e0aa5ca5a149291ee4300c3281d38b2422c73ad89f256b2fe102ca0c6`

Final `adb devices -l` returned an empty device list. No artifact was installed and no ARM64 contract executable could be run on the authorized `SM-S948B`. Adding a separate JNI/test executable was not necessary to resolve code ambiguity: the preserved normal APK contains the exact audited production objects, both ARM64 configurations have symbol-range disassembly proof, and the real production C++ fixtures pass. Exact phone execution, artifact parity, Mobile/High 75%, separate 100%, warmed timing/thermal/GPU-power/resources, and Home/resume remain the hard Task 9 device gate. No device evidence is claimed. Hands-on perceived glass quality remains an owner boundary. Task 5's two-cut combo owner audio/haptic replay remains separately open and unchanged.

Audio/haptic manual revalidation required: NO — this milestone changes material transport and shader visibility only; audio/haptic state, event timing, playback, spatialisation, and feedback semantics are unchanged.
