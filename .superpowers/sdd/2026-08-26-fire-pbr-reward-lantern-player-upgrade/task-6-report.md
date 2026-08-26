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
- **Unclosed volume** means a refracted path reached an ordinary terminal with its closed-volume stack still nonempty and was deterministically rejected. Temporary instrumentation first proved all 38 probed events were ordinary-terminal-with-open-stack, not no-hit. A second probe recorded first-interface incidence: maximum cosine 0.185004 and summed cosine 0.900348 over 38 events, mean 0.023693. Thus these are recorded silhouette/grazing paths, not an unexplained central miss. The final standard count is 36; camera/fire sampling changes make fire and tinted 53; the deliberately grazing edge checkpoint is 500. The same closed fixture scaled to exactly 1 mm has zero unclosed events in direct through-view. Temporary probe encoding was removed; the table contains the restored split counters.

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
