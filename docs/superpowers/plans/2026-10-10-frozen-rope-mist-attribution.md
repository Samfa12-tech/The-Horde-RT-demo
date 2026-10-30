# Frozen rope/mist attribution

The closer frozen reproduction shows the same kind of detached angular band seen in the owner's images, distinct from the connected floor shadow. Hardware queries attribute that reproduced band to rope/sky visibility reused from the mist midpoint. There are real short rope-shadowed intervals: the midpoint method applies their blockage to the whole segment, while six sample-local queries can miss them entirely. **Neither brighter output nor removing the band is sufficient correctness evidence.** The floor-shadow path is preserved. Exact original screenshot camera/rope/module identities were unavailable; the supplied images' complete appearance is not certified as an exact recovered frame.

Production code, shaders, ABI, packages, masks, geometry, density/extinction, sample counts, quality and every budget remain unchanged. This is an evidence-only checkpoint on `codex/horde-1.7-wp2-vertical-proof`, continuing draft [PR27](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/27). Starting local and remote HEAD matched `f2a095fcf85520d63aebc1a48547cf4a453e97ba`; tracked worktree was clean. The primary checkout and other worktrees remain untouched.

The independent seven adversarial fixes were already finished at [d5e4b6a](https://github.com/Samfa12-tech/The-Horde-RT-demo/commit/d5e4b6ae83cec4f3cc4111226a16c34355d1ff87), with immutable evidence at `f2a095fc`. Their Windows Debug/Release affected native selections passed 28/28 each; Android four-ABI builds/lint/selected Java checks and six GitHub checks passed. Those are **historical results**, not checks repeated by this mist experiment. See the [independent-fix report](2026-10-10-pr27-adversarial-review.md).

## Frozen state and scope

This is a reproducible staged offending frame, not recovery of the owner's exact screenshot. The owner receipt contains no camera pose or rope-node freeze. Its recorded lantern position differs. The primary series matches its 1232×803 resolution, 100% scale, High fire/water, Higher surface shadows (4 local / 2 sky), Standard dust and mist-on settings. An earlier Current-shadow 960×540 exploratory series is retained separately and is not presented as a matched owner frame.

- Source: `f2a095fcf85520d63aebc1a48547cf4a453e97ba`; production Windows Debug EXE remains `1aae10d210842bd789b5ec8e9d939d68831c04bd5b41d144bea44421a1d2f094`.
- Frozen simulation tick 601 after 600 shared 60 Hz steps. Camera `(-33, +0.70, -17.5)`, yaw `-2.805`, pitch `-0.24`; support `-0.95`, height delta zero; no advancing simulation during the paired captures.
- Rope Ready / LowerSafe, 12 identical nodes, 176 identical triangles; anchor `(-33.7000008, 3.67799997, -15.5)`, tension `70.2491989`. Every shaded capture and query-map capture has identical native CRLF frozen-state SHA-256 `c0504e981daeeb528185a95c1dd19cbfcf6d7244bfb635c913abfab0e67e63b8`.
- Lantern claimed and held High. The actual uploaded renderer emitter is ID `0x4c414e54`, position/strength `(-33.0775375, 0.298102945, -16.8894691, 0.686399996)`, colour/intensity `(1, 0.426759988, 0.0815246254, 0.686399996)`. This renderer-appended emitter is not the inactive simulation torch record.
- Staff strength zero, roof open 1, dawn reveal zero. The frozen sky source is the existing cool opened-roof aperture pair at `(-34.35, 2.76, -16.02)` and `(-33.05, 2.76, -14.38)`. The game also has an ordinary directional moon, but `activeSkyLight` selects the authored aperture branch in this state.

The hidden diagnostic executable links the existing native renderer/simulation libraries. It substitutes explicitly measured temporary GenericDielectric modules and a copied capture entry; production provider and packages are untouched. The all-light midpoint SPIR-V is byte-identical to production. The probe provider uses the required canonical logical roster path under its diagnostic artifact root, with actual temporary word/include hashes. Its canonical key is not a claim that isolated modules were admitted to the frozen catalogue. OpaqueFast stays the original production module; the active physical strategy is GenericDielectric only.

Shaded probe EXE: `018ed89dd2b4bf5bb311c63161b6479cc778890082e6da2c61a8a69f92dd882c`. Query-record EXE: `45d3ab6494cc9b4938f36118c6bbaf36d397d1d5d095e9e758759766c51b3eb5`. Both are experimental, unshipped Windows Debug artifacts. [Exact toolchain and tool hashes](../../evidence/2026-10-10-frozen-mist-attribution/toolchain.json), [native/input identities](../../evidence/2026-10-10-frozen-mist-attribution/input-identities.json), [immutable production source links](../../evidence/2026-10-10-frozen-mist-attribution/source-identities.json).

## Six-way comparison

Only mist incident sources are isolated: all-light, sky-only and the admitted lantern-only. Surface illumination/shadows remain live in every image. Sample-local recomputes source direction/distance and visibility through the **same hardware helper and mask `0x35`** at each existing density sample. Density, extinction, point attenuation, authored 2/6/8 sample positions, camera, scene, materials, masks and presentation quality are preserved.

| Mist probe | Active SPIR-V SHA-256 | Original capture exit / wall seconds |
| --- | --- | --- |
| Midpoint all | `223706ea208242f34770b919e14a8a6ae1391ff187820325d9759fc28dee6795` | 1 / 7.5675 |
| Midpoint sky | `81b7a1e3a58ccd5c8ca1542be767a57925908cd944ba20646b5190301409b7d7` | 1 / 3.6253 |
| Midpoint lantern | `41ec3eea66114e1a7d2f7b1e09e1017c66c215aa39612781bbb88db67d53e975` | 1 / 3.6012 |
| Sample-local all | `0603ab67ccebc408017f8be80b4b42379f02911509b5f635cc9417a8a831e82a` | 1 / 5.2928 |
| Sample-local sky | `be20163adfaa33ceb563b0580bdbe977df3b1a9d5348522ae5acf6850c098edb` | 1 / 4.0253 |
| Sample-local lantern | `3cd4cd0d86abc417ff893373ed600563c78267229ea31c2316e936a0ce26bf6f` | 1 / 4.1884 |

Each adjacent canonical `completed-frame.json` proves real Pipeline dispatch, swapchain copy and successful presentation of submission 12 at frozen tick 601. **Every original showcase admission nevertheless failed**, retaining the strict claimed-reward primary-visibility check: sword/torch counts `0/7918`, player/ring/body `109928/997/41645`. No original assertion was bypassed or weakened. The image and completed-frame evidence are written before that later rejection. They support the experimental comparison, not a passed showcase, rig, or owner acceptance gate. The staging/camera differs from ordinary play; this failed framing result is not classified as a newly reproduced live-game defect.

Across 989,296 pixels, midpoint all and midpoint sky differ at only one pixel by one green byte; sample-local all and sample-local sky are identical. Lantern-only midpoint and sample-local images are identical. All-light midpoint versus sample-local changes 130,010 pixels, with maximum channel difference 49. These are measured output differences, not a new acceptance tolerance. [Full pixel ledger](../../evidence/2026-10-10-frozen-mist-attribution/pixel-metrics.json), [commands, exits, elapsed time and environment](../../evidence/2026-10-10-frozen-mist-attribution/runs.jsonl).

## Hardware attribution

Two additional diagnostic query maps encode the **existing committed intersections**, without additional query sites, mask exclusions, geometry removal or changes to the helper's returned transmittance. They are data readbacks, not quality/visual acceptance captures. A read-only accessor in the copied header records the actual world primitive count and masks; it changes no object layout or production ABI. BGRA readback channel order is explicitly decoded in the [attribution ledger](../../evidence/2026-10-10-frozen-mist-attribution/attribution.json).

The renderer world contains 3,204 primitives; the final 176, indices 3028–3203 on instance 0, are the actual dynamic rope. Hardware sky queries report that rope as the blocker at **6,722 pixels**. Of those, **6,427 become brighter** under sample-local visibility; none become darker. This establishes actual rope/sky query participation in this staged frame. It does not yet quantify which density samples should remain dark, or prove that the owner's specific band is misplaced, incomplete or too strong.

![Unresampled rope-band crops: midpoint, sample-local, actual hardware rope blockers](../../evidence/2026-10-10-frozen-mist-attribution/comparison/rope-band-comparison.png)

The lantern query map reports nonzero tuned raw radiance (UNORM8 maximum 175) but returned visible radiance quantized to zero at UNORM8 precision for all 469,920 active pixels. Of these, **462,427 are blocked by lantern body instance 8**, 7,470 by world instance 0, 21 by instance 2 and two by instance 4. For the committed opaque blockers, the unchanged helper returns exactly zero. This explains the lack of visible lantern mist contribution **in this pose**; it does not prove the lantern never casts shadows elsewhere. Do not fix it by disabling body masks, surface shadows, or moving a light independently of its physical rig. A separate emission-boundary/attachment investigation is needed.

The full original surface-shading functions, helper, medium formulas and integration loops are byte-preserved in the six shaded source transformations. An independent read-only review verified that isolation scope. The canonical frames retain equal scene/resource counts, ray masks and uploaded lantern inputs. Single-frame GPU timestamp values are retained in the frame JSON; they are not sustained performance, production capacity or Android evidence.

## Owner-facing reproduction, CURRENT/HIGHER and continuous blockage

After the owner clarified that placement, strength and continuation matter, a second frozen viewpoint was staged at camera `(-34.4, +0.70, -13.5)`, yaw `+0.10`, pitch `-0.24`, with the lantern Low. It places the rope right of centre, the pillar/chest to the left, and a detached angular band crossing the mist separately from the floor shadow. This is a visually similar reproduction, **not exact camera recovery**. The two supplied [image identities](../../evidence/2026-10-10-frozen-mist-attribution/owner-reference-identities.json) retain CURRENT as owner-confirmed and image 2 as **probably HIGHER**, not verified. Rope geometry, tension, time and readiness remain frozen at tick 601; the actual lantern emitter is now `(-34.473011, 0.115620866, -14.0590429)`, strength `0.686399996`.

Both viewpoints now have twelve fresh shaded comparisons: CURRENT/HIGHER × midpoint/sample-local × all/sky/lantern. Each viewpoint uses one executable for both settings and all six modules. The closer view has native frozen-state SHA-256 `b19c4e07b17725ae920907974310240ff130d5b8ddf25fd28a771a9067dcc7f7`; all of its shaded and weighted probes agree. [Exact additional executable/input/module identities](../../evidence/2026-10-10-frozen-mist-attribution/extension-identities.json), [argv, settings, raw/published state hashes, exits and timings](../../evidence/2026-10-10-frozen-mist-attribution/extension-runs.jsonl).

![Same frozen view: CURRENT/HIGHER midpoint versus sample-local](../../evidence/2026-10-10-frozen-mist-attribution/comparison/owner-view-current-higher.png)

CURRENT uses 1 local / 1 sky surface sample; HIGHER uses 4 / 2. Critically, `areaShadowSampleIndex()` also changes **mist source selection**: CURRENT alternates aperture point 0/1 by pixel parity, while HIGHER forces point 0. Mist does not average the two sources just because surface shadows use two sky samples. The visual comparison therefore includes both surface-quality and mist-aperture selection differences, not a pure sample-count experiment.

In the closer view, all-light versus sky-only differs at one pixel/one green byte in both midpoint settings. Lantern-only midpoint/local pairs are identical. The source isolation leaves all surface shading, geometry and shadow paths live. Sample-local all-light changes 113,776 CURRENT pixels (**39,748 brighter, 74,028 darker**) and 159,928 HIGHER pixels (**28,234 brighter, 131,694 darker**). This disproves the simple interpretation that the experiment merely reduces darkness. [Complete same-view comparison ledger](../../evidence/2026-10-10-frozen-mist-attribution/owner-view-comparison.json); [first-view comparison](../../evidence/2026-10-10-frozen-mist-attribution/quality-comparison.json).

Five extra sky-only data modules measure source-normalized scattering loss, weighted by **unchanged density, step length, source radiance and prior extinction**. They encode exact float32 bits through the existing RGBA8 image/PNG, including alpha; the mask module records the actual six sampled committed intersections. No SSBO, production ABI, query mask, density or surface path changes. Small float roundoff is retained, not clipped in the numeric ledger. Sentinel -1 means no positive integrated raw sky contribution. These are data readbacks, not visual-quality frames.

| Closer-view rope footprint | CURRENT | HIGHER |
| --- | ---: | ---: |
| Midpoint rope-shadow pixels | 7,339 | 10,361 |
| Sample-local rope-shadow pixels | 36,860 | 51,647 |
| Midpoint/local rope overlap above diagnostic roundoff | 650 | 0 |
| Actual rope intersections at six density samples | 38,586 | 51,735 |

These unequal footprints identify **placement/continuation aliasing**, but zero hits at six samples do not prove a continuously clear segment. The [weighted ledger](../../evidence/2026-10-10-frozen-mist-attribution/weighted-shadow-metrics.json) preserves both actual blockage and its contribution; displayed grey maps are only visualizations of the exact float payload.

![HIGHER: midpoint rope loss, sample-local total loss, sample-local retained rope loss](../../evidence/2026-10-10-frozen-mist-attribution/comparison/weighted-higher.png)

To check that distinction without changing render quality, a bounded **CPU geometric oracle** intersects camera rays with analytic point-light shadow cones from the actual 176 frozen rope triangles. Thirty-two selected clear-floor rays agree with the real hardware midpoint decision and six-sample rope mask (**32 agree, zero rejected**). Its 64/256/1024/4096-step numerical integrations are diagnostic CPU quadrature, not new runtime sample counts, GPU measurements or whole-volume convergence acceptance. It isolates rope geometry; other occluders and area-light mixtures remain outside this oracle. All selected rays use aperture point 0, so this subset does not certify point 1. Clear-floor depth is a selected visual/geometry premise: actual primary scene depth was not separately captured, and mask agreement alone does not prove that depth. The copied staging guard requires zero dawn; the CPU density formula relies on that state.

For eight band rays in each setting, midpoint loss is approximately **100%** of the sky contribution, six-sample rope loss is **zero**, yet the continuous rope-shadow interval exists. The 4096-step oracle estimates **2.98–5.69%** source-normalized rope loss. At pixel `(838,574)`, the actual blocked interval is about `0.09375 m` within a `2.45185 m` medium segment; its estimated contribution loss is `3.91%`. Selected continuation rays also have real blocked intervals despite a clear midpoint. [All rays, intervals, six-bit agreement and convergence values](../../evidence/2026-10-10-frozen-mist-attribution/continuous-rope-oracle.json).

This explains the reproduced visible defect more precisely: a legitimate thin shadow is represented as a detached full-segment band, with missing or misplaced continuation elsewhere. The naive local reference replaces it with sparse sampled ribbons and can erase genuine thin blockage. It is **not an accepted fix**. The independent floor-shadow implementation and light/geometry/mask ownership remain untouched. Confirmation on the owner's exact original freeze and motion remains open.

## Reusable correction boundary and frozen budgets

The appropriate contract is source-independent: represent the contribution of actual blocked intervals, with bounded sampling/reuse that preserves thin-shadow strength and continuation. A single binary midpoint and a naive six-point reference both fail that contract in this reproduction. Keep the shared hardware transmittance helper, physical occluders and surface-shadow path. No rope-specific exemption, extra light, density tweak, camera compensation or quality reduction is justified.

A full sample-local reference was compiled and validated for all eight existing Pipeline keys using each recorded strategy and optimizer sequence. It is **not publishable**:

| Key | Baseline bytes | Sample-local bytes | Baseline → reference static query sites | Frozen admission |
| --- | ---: | ---: | --- | --- |
| diagnostic_high_generic_dielectric | 295960 | 295832 | 2 → 2 | Fits static bounds |
| diagnostic_high_opaque_fast | 595520 | 830368 | 29 → 74 | FAIL |
| diagnostic_mobile_generic_dielectric | 294460 | 294332 | 2 → 2 | Fits static bounds |
| diagnostic_mobile_opaque_fast | 592420 | 827268 | 27 → 72 | FAIL |
| shipping_high_generic_dielectric | 287132 | 287004 | 2 → 2 | Fits static bounds |
| shipping_high_opaque_fast | 591284 | 826132 | 29 → 74 | FAIL |
| shipping_mobile_generic_dielectric | 285632 | 285504 | 2 → 2 | Fits static bounds |
| shipping_mobile_opaque_fast | 588184 | 823032 | 27 → 72 | FAIL |

Each OpaqueFast row exceeds **bytes, words, instructions, branches, loops, selection merges and static query initializations**, and fails the registered `driverSafeFullyInlined` exact invariant because its query sites exceed the emergency shape bound. Merely retaining one function/zero calls does not pass that invariant. Generic keeps 83 functions, 264 High / 261 Mobile calls and two sites. Diagnostic/Shipping atomic counts and diagnostics binding remain correct. [Every before/after metric, invariant and violation](../../evidence/2026-10-10-frozen-mist-attribution/sample-local-full-metrics.json).

Static size is not hardware memory capacity or measured frame cost. Dynamic work also grows: with this sky plus one lantern, Authored mist can execute 12 source queries rather than two per intersecting ray; the general six-source ceiling becomes 36. A smaller retained Generic module does not establish an efficient or portable correction. No Freeze, package publication or production fallback was attempted.

The next implementation decision is a bounded interval-aware/sampling-reuse design that fits **every existing maximum and invariant**, checked against the weighted and geometric witnesses, then motion and both hardware backends. The straightforward reference is ruled out as a production patch on correctness and static admission grounds. Lantern self-occlusion in the first pose is a separate measured issue; do not bundle a mask bypass into mist sampling. No budget adjustment is requested or made.

## Reproduction, retention and remaining gates

From the existing worktree with the recorded Debug libraries/toolchain, the [saved experimental scripts](../../evidence/2026-10-10-frozen-mist-attribution/reproduce/prepare.py) generate only ignored `reports/frozen-mist` output. Verify [library hashes](../../evidence/2026-10-10-frozen-mist-attribution/input-identities.json) and unchanged source/dependencies first. Use `python docs/evidence/2026-10-10-frozen-mist-attribution/reproduce/prepare.py`; then run the sibling `capture.py` with modes 0–5. It explicitly selects the diagnostic mode, uses a fresh suffixed output directory, remains hidden, sends no input and retains the nonzero original showcase exit. `prepare-records.py` / `capture-records.py` produce the two query-data modes. These are capture experiments, not a shipped alternative backend or general fixture admission policy.

The registered baseline command was `pwsh -NoProfile -File tools/compile-raygen.ps1 -Matrix -OutputDirectory <repo>/reports/frozen-mist/matrix-baseline`. All eight fresh production SPIR-V hashes, dependency identities and dependency lists match the unchanged catalogue. The reference script uses those resolved inputs; full preprocessed/resolved GLSL, SPIR-V, validation logs and disassembly remain under ignored reports, with [exact retained identities](../../evidence/2026-10-10-frozen-mist-attribution/retained-shader-identities.json). Public logs are sanitized. Native text output is retained unchanged locally; public copies use LF, with separate published-byte identities. Private paths, device identifiers, desktop content and raw private logs are excluded.

Setup failures are retained: native include/runtime/manifest configuration errors, pre-initialization provider rejection, and rescue geometry initially enabled too late. The corrected renderer is initialized with rescue geometry before the primary matrix. The record-map directory setup failure and the corrected metric derivation are also documented; no failed attempt is promoted to a pass. [Attempt ledger](../../evidence/2026-10-10-frozen-mist-attribution/retained-attempts.json), [receipt](../../evidence/2026-10-10-frozen-mist-attribution/receipt.json).

The extension's exact saved scripts are in the same reproduction folder. Copy the scripts into ignored `reports/frozen-mist` before replaying creator scripts, because they intentionally consume the preserved original preparation scripts. Prepare the original shaded and record harness inputs first; then `create-quality.py` / `create-owner-view.py` build their child preparation scripts with `--native-only`. `prepare-weights.py --owner-view` and `--owner-view --midpoint-rope-only` produce the two weighted families. Every capture requires an explicit mode and CURRENT/HIGHER environment selection. Inspect recorded identities before replaying; do not silently substitute other libraries, SDKs or production packages.

No runtime/test/asset changes require a new gameplay matrix, torch sweep, Android build or package re-admission here. The original torch test and its 180-second policy are unchanged; the latest 171.84-second Debug pass remains historical. Fresh shader compilation/validation and 42 successful diagnostic presentations supplement that evidence, while **all 42 original showcase admissions remain failed**, without transferring owner acceptance. Android, Query, continuous traversal/bump, lifecycle, exact original-owner-frame recovery, latest independent-fix playtest and audio/haptic acceptance remain open. No device installation, phone operation, gameplay input, merge, release or next work package occurred.
