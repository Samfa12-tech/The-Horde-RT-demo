# Near-corner dielectric continuation fix

Status: **demonstrated defect repaired; Phase4 glass acceptance still open**.
Built from `5935db9` plus the recorded four-file shader/CPU/test change. The
separate harness correction is `bbb00f0`. These are exact development artifacts,
not a clean final release candidate, publication, or a performance improvement.

## Cause and bounded change

[The recorded native ray](../2026-09-27-glass-path/README.md) skipped a valid
side exit near a pane corner. Pane thickness alone does not bound that distance.
Remove the tangential/direction component from dielectric origin bias; retain
the existing scale-aware outward-normal bias. Decouple the next query minimum
from that bias, using1 micrometre in both generic and compact production routes,
for reflection and transmission. The CPU mirror and regression use a3.08mm
pane with a20-micrometre side exit. The old implementation failed two assertions;
the candidate passes. This is not a proof of every sub-micrometre/wedge case.

No material, geometry, ray mask, budget, Fresnel, IOR, refraction, TIR,
attenuation, diagnostic counter or recovery policy was changed. The accepted
world/viewmodel assets and animation remain unchanged. No investigation output
override or relaxed shader footprint budget remains. A focused independent
source review found no correctness blocker; lead review confirmed the four
query sites and added explicit regression coverage for their minimum arguments.

## Fresh Windows RTX evidence

GPU: RTX5050 Laptop. Three matched540x960 captures per quality, each exact
baseline/candidate executable, Diagnostic RayTracingPipeline. Mobile baseline
EXE `f228b72a355a5e132c1477ef0cb415e362588c257eb51f6c7f1923d16b784a21`,
candidate `dbfc90d5e3f8bd3a538ad808d44aba58e0a035e27938708d23ba97effd5adad0`.
High baseline `0af9d37b6cdab15967124a28143b2241be2e241a7955609d43fa1afa06dfddfe`,
candidate `c52e789ebad2e6439c750ff746bf4fce92c273dcb8d202e28391c88218cb1263`.

| Quality/view | Transport before→after | Shadow before→after | Certified recovery before→after |
| --- | --- | --- | --- |
| Mobile isolated lantern | 1→0 | 4776→4759 | 25→20 |
| Mobile high/look-up | 0→0 | 1113→1110 | 116→109 |
| Mobile low/parry | 0→0 | 8166→8166 | 0→0 |
| High isolated lantern | 1→0 | 0→0 | 6→0 |
| High high/look-up | 0→0 | 47→46 | 10→0 |
| High low/parry | 0→0 | 0→0 | 0→0 |

All manifests complete; near-self and unclosed counters remain zero. These
are correctness captures, not matched Shipping performance. Desktop Mobile
shader selection does not imply every water/runtime control matches Android.

Standard13 comparison:12 images byte-identical. **Finale-roof fails the existing
image gate**:82/518400 pixels differ by more than1 (fraction.00015818), maximum
channel delta240. The threshold remains max3/fraction.001, with no new baseline
approved. Largest delta is lantern pixel(425,395), dark `(9,4,1)` becoming
`(249,216,78)`; other deltas include transmitted background and lit surfaces.
This is consistent with changed continuation/secondary origins, not proof of
all-pixel correctness. Keep the failed report and old/new images. Comparison
timing happens to pass but is not matched Shipping evidence or a gain claim.

## Exact SM-S948B evidence

Android16, Adreno840, driver2150932499, 75% RT scale1080x2235, presentation
1440x2980. Baseline ordinary Debug APK:
`14927941cc7e7596943b0a2092c01ae1ff30277d376697e170c1d44381057688`.
Candidate ordinary Debug APK:
`fd11d9dbe11d2087b60b56fb7446d281afde93d908b175ea6124b699f989df4b`.
Installed/pulled APK hashes agree. App data and separate accepted viewmodel app
were preserved. Strict ASTC and native RayTracingPipeline presentation confirmed.

| View | Transport before→after | Primary volume before→after | Certified recovery before→after |
| --- | --- | --- | --- |
| Isolated lantern | 11→1 | 10→0 | 98→80 |
| High/look-up | 8→0 | 8→0 | 543→528 |
| Low/parry | 1→0 | 1→0 | 48→0 |
| Millimetre closed | 0→0 | 0→0 | 12→0 |
| Edge Fresnel | 0→0 | 0→0 | 30404→30081 |
| Tinted transport | 0→0 | 0→0 | 1→0 |
| Fire transport | 0→0 | 0→0 | 1→0 |

Shadow/near-self/unclosed counts are zero throughout these phone captures.
The isolated lantern's remaining one interface-budget failure is unchanged;
large certified-recovery counts, especially grazing glass, remain unresolved.
The nineteen removed volume errors were not merely shifted into recovery.

Baseline run062025 remains overall **failed** for its stale tinted-zone
expectation. Affected same-APK fire/tinted rerun062520 passes; candidate run062611
passes all7 captures and Home/resume. See [harness correction](harness.md).
These runs contain no timed benchmark. Native captures, states, summaries and
reports retain their original bytes under `phone/` and `windows/`; only the
three focal phone PNG pairs are retained, other capture hashes remain in manifests.

Same exact candidate's `glass-corner-live-20260930-01` completed the existing
`lantern-reveal-sequence-v1` workload at75%, with real fixed-step60Hz simulation,
600 warm-up and600 measured frames. All600 measured rows have presented outcomes,
valid CPU/GPU/Diagnostic status and matching submission/completion identities;
zero rejected/cancelled/outstanding rows. Exported originals are in
`phone/live-reveal/`. This verifies live-sequence execution and completion, not
visual motion acceptance: the report carries Diagnostic availability but **not
per-frame glass counter values**, and no continuous video was collected in this
run. Do not infer zero live transport failures. Debug timings lack a matched
baseline/thermal trace and are not a Shipping performance comparison.

## Build and shader checks

Fresh targeted Vulkan-enabled MSVC Debug CTests7/7 passed (artifact ownership,
variants/provider/bundle, dielectric math, static GLTF and character slots).
Shared gameplay zone test and PowerShell capture contracts also pass.
Android ARM64 Debug and unsigned Release build/package checks pass. Release APK
`7156eb44d8b49d1b51c7e55b5c9816692f200fab5db95f2248a6851a7c021d9c`
was not installed or published. Debug/Release packaged ARM64 libraries match
their stripped libraries and validate/disassemble successfully.

Compatibility modules and all16 pipeline/compute variants were regenerated
against unchanged footprint budgets. Shipping Mobile opaque/generic words:
pipeline125253/54760, compute125318/54823. All four packaged Shipping modules
contain **zero atomics and no Diagnostic binding22**. Generic modules are36words
smaller than before; that is not measured performance improvement. Packaged
shader reports are retained in `checks/`.

The full artifact-contract script also passed, including four per-query negative
minimum-distance mutations, fresh eight-key compilation/catalog agreement and
both compatibility module freshness checks. Its old compatibility hash pins
were already stale atbbb00f0; they now explicitly pin the reviewed September30
artifacts. No footprint, descriptor, pixel or physical budget gate was relaxed.

## Still open / next work

- Preserve the failed image gate until affected changes are fully reconciled;
  do not loosen thresholds or label this an image-equivalent optimisation.
- Diagnose remaining interface-budget/certified-recovery and shadow failures;
  no scalar-glass/shadow fallback is accepted as a solution.
- Live-motion glass correctness and matched Shipping comparisons remain separate
  from frozen poses. Shipping/Diagnostic and pipeline/compute parity are separate.
- Enforce disjoint-component admission where compact transport relies on it.
- S24/S25 acceptance, remaining programme features and final matrix remain open.

Audio/haptic manual revalidation required: **NO** for this shader/harness slice;
no event-time inputs, feedback transport, playback or haptic changes.
