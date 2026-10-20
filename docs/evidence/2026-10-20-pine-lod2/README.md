# One-pine LOD2 comparison — rejected for production adoption

20 October 2026, existing worktree
`C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.7-wp2-vertical-proof`,
branch `codex/horde-1.7-wp2-vertical-proof`, existing draft PR 27. Base `d44fd8f2`;
[source freeze](source-freeze.json) pins the actual uncommitted capture sources.
The Windows Debug executable was 14,029,824 bytes, SHA-256
`6c4b2cb1b2953a1815543cf519a2082f892c6eb9e4f41dc02f4b4a3ae5387e78`.

## Decision

Keep the admitted LOD1 pine. The owned derivative remains **unadmitted**, with
its deterministic recipe and failed/accepted verification results preserved.
This diagnostic does not add production LOD, wind or a new asset/scene owner.
The small resident saving and uncertain steady GPU result do not justify the
measured whole-world-owner replacement cost. Stop this experiment here.

The retained implementation is reusable **Debug inspection and comparison
tooling**: complete cached-world primitive inspection, strict native span and
provenance checks, and baseline/candidate/restored completed-frame sampling.
Three developer views use existing accepted-route stations and current real
ascent/walking staging. Shipping assets, collision, saves, lighting/settings,
shader quality and near-tree geometry stay unchanged. No phone/public action.

## Owned derivation and identity

The existing owner-authored source is
`assets/models/world/runtime/forest-tree-pair-v1/horde-irregular-pine-v1-lod1.glb`,
SHA-256 `db2d45547ad2f629c48ab8c994db845eb5d097a7db5bed6192b837e29f7a1b3f`.
The [derivation receipt](derivation-receipt.json) pins its original authoring
script/rights statement. No provider, third-party geometry or new licence.

- Remove 96 of 366 **complete closed opaque needle lenses**, preserving bark,
  roots, moss, texture/material payloads and all kept vertex attributes exactly.
- Whole tree: 5,000 → 4,232 triangles; 5,602 → 4,642 exported vertices. GLB
  1,402,952 → 1,352,264 bytes, saving 50,688 bytes. Candidate SHA-256
  `181e8e447ea4994b0851bdc6706ba5eaf1d5ef7784fb88cdc0de48e43885cef0`.
- Bounds, 24 horizontal band hulls and 936 tested support extrema remain exact.
  This does **not** preserve projected density: CPU foliage coverage loss averages
  13.29%, worst 15.53% across 36 projections. Closedness is independently checked.
- glTF Validator 2.0.0-dev.3.10: zero errors/warnings, two unused-tangent infos.
  Local `game-dev` CLI was unavailable; deterministic Python, installed glTF tools
  and the native cached-source inspector were used. No package admission claim.

The [native mapping verification](native-mapping-verification.json) proves the
selected fourth pine, placement 7, rather than assuming absolute index offsets.
Cached owner 21 / ForestPine primitive row 25 starts at triangle 293,653.
All 32 ordered index repetitions and UV bytes match; all 93,696 family triangles
carry ExteriorDressing provenance. All 3,660 selected-pine vertices and 2,928
triangles were checked. Maximum reconstructed vector residuals are 1.39313e-6 m
for position, 1.66060e-7 normal and 1.66859e-7 tangent, under unchanged 1e-5 limits;
reconstructed float arithmetic is not claimed bit-exact. The source inspector
also compares cached vertex/index bytes exactly with the assembled native owner.

The [768 omissions](global-omissions.json) are primitive-local source needle
triangles mapped by actual runtime offset + 8,784 + local ID, range 302,445–305,220.
No other tree, bark/root, collision or material is selected. Native RT compares
the exact retained cached attributes/triangles; it does not import/admit the
repacked candidate GLB. The compact GLB's potential native vertex/index saving
would be 70,656 bytes; **that saving is not realized by this comparison path**.

## Matched native desktop result

RTX 5050 Laptop GPU, driver 610.47. Diagnostic/High, 960×540, scale 100%,
High water/fire, Higher shadows, Windows Mist On; frozen simulation clocks and
ordinary equipment. Each phase discards 12 warm frames and collects 60 unique
canonically valid completed RT submissions: 1,080 retained samples in six runs.
Per-frame device idle serializes these observations. They are neither ordinary
frame pacing nor sustained/warm/mobile acceptance. No rolling average is used.
See [per-frame CSV](samples.csv) and [complete summary](comparison-summary.json)
for serials, identity, CPU stages, p95, resources and hashes of original receipts.

GPU command-buffer milliseconds, median / nearest-rank p95:

| Backend / distance | Original | Candidate | Original restored |
| --- | --- | --- | --- |
| Pipeline / 5.23 m | 1.980 / 1.995 | 1.982 / 1.993 | 1.977 / 1.991 |
| Pipeline / 18.70 m | 2.561 / 4.120 | 2.553 / 2.571 | 2.552 / 2.562 |
| Pipeline / 24.36 m | 4.190 / 4.282 | 2.695 / 3.044 | 2.450 / 2.471 |
| Ray query / 5.23 m | 1.946 / 2.538 | 1.946 / 1.953 | 1.949 / 1.957 |
| Ray query / 18.70 m | 2.605 / 3.848 | 2.842 / 2.884 | 2.765 / 2.780 |
| Ray query / 24.36 m | 2.388 / 4.025 | 2.490 / 2.498 | 2.835 / 2.858 |

The apparent far Pipeline improvement (-35.70%) is **not accepted**: the
pixel-identical restored original improves still further (-41.53% vs initial),
and the candidate is 9.97% slower than that control. Far query's restored control
drifts +18.75%. Near results differ by about 0.1%; mid query is worse than both
controls. Canonical identities, shaders, poses, resource counts and restored
images match, so changed visible content does not explain control drift. Driver/
clock/cache state was not held fixed or diagnosed; two incidental desktop
observations (44°C and 47°C) do not establish matched thermal/clock state.
There is no reproducible steady-state win to bank and no reason to extend a
trial already rejected for its transition cost. All GPU medians lie within the
16.667/20/33.333 ms **GPU-only** bands; this is not an FPS claim.
CPU samples are not uniformly smooth: far query's restored phase retains a
491.036 ms whole-frame CPU outlier (render call 491.0418 ms) in the CSV/summary.

Actual candidate resident change in every run:

- Device local: 372,437,216 → 372,380,128 bytes, saving **57,088 bytes (0.0153%)**.
- Host visible: 48,278,656 → 48,269,440 bytes on Pipeline, saving **9,216 bytes**.
  Query has 512 fewer baseline host bytes and the same saving. No vertex payload
  is removed; the full 46,871,933-byte cached CPU source remains available.
- Candidate preparation spans 102 RT preparation frames, 605–1,117 ms elapsed;
  6,372,748 bytes uploaded and the whole collapse BLAS/TLAS rebuilt. These are
  preparation costs, separate from the steady samples.
- Candidate adoption **53.67–65.64 ms**; device overlap +32,981,504 bytes and
  host overlap +6,372,928 bytes. Two temporary TLAS owners return to one.
- All six complete restorations recover exact baseline inventory: Pipeline
  51 buffers / 60 allocations / 20 BLAS / one TLAS / 28 instances; query 49/58,
  same AS/instance counts. No retained growth across either cutover.

## Visual inspection and remaining gates

Inspected the six native A/B/A comparisons, including actual RT shadows. Every
restored image is **pixel-identical** to its original. Candidate changed pixels
are 887 near, 247 mid and 50 far on both backends; differences include thinning
and real ground-light/shadow changes. Lower-image changes are 537/34/1 pixels;
that region includes held props and is not an object/shadow mask. See the raw
local originals and comparison contact sheet under
`reports/pine-lod2-20261020/`; no PNG, GLB or binary dump enters Git.

The mid/far crown retains its coarse outline but loses interior coverage. The
near production-pitch view crops the upper crown; whole near-tree acceptance is
therefore incomplete. These are explicit static cutovers at three actual route
distances, not moving/hysteretic distance-LOD transitions. No runtime threshold,
shadow continuity during travel, foliage wind, combined exterior workload or
physical mobile acceptance is closed. Current LOD1 remains the ordinary version.

The hidden process runner checked settings hashes before/after each run; all
processes exited zero. Foreground API samples were null/unchanged in this
execution context; do not infer another application's identity from that.
The 45-second in-function deadline is cooperative; Vulkan fence/acquire/idle
calls can block. The outer runner observes 90 seconds and reports pending rather
than killing/retrying a process. No deadline/pending failure occurred.

## Checks, rejected checks and reproduction

Windows Debug app build and four affected Release CTests pass: development
registry, static-geometry cutover, region source and performance evidence (4.00 s).
Android was not rebuilt: this adds Windows Debug tooling and development views,
with no production renderer/simulation/asset change. No phone action. Audio/haptic
manual revalidation required: **NO**, no event, listener or playback change.

The first build passed but preceded the final inspection output-root guard by
six seconds; it was rebuilt before capture. Both logs are retained. The initial
offline mapper falsely required walkable root-tip ground; native tree placement
requires found support and permits slopes. Its rejected script/log are preserved;
the correction matches that authority without widening tolerances. The initial
summary helper used the wrong `cpu` JSON key; its failure remains, and the corrected
`stages` reader generated the published summary. No rejected run is relabelled.

Copy the exact scripts in [reproduce](reproduce/) and the pinned
`reproducibility-receipt.json` into a fresh
`reports/pine-lod2-20261020/` on an equivalent checkout (archive existing outputs
separately first; scripts require fresh native outputs). From the worktree:

```powershell
python reports/pine-lod2-20261020/derive_pine_lod2.py
python reports/pine-lod2-20261020/run_pine_compare.py inspect
python reports/pine-lod2-20261020/verify_native_mapping.py reports/pine-lod2-20261020/native-inspection
python reports/pine-lod2-20261020/run_pine_compare.py compare
python reports/pine-lod2-20261020/analyze_pine_compare.py
```

Use the pinned derivation/reproducibility receipts alongside the scripts for the
mapper's input identity check. Local installed Pillow, shader SDK/toolchain and
the exact Debug app build are required; no download/provider call is needed.
Raw canonical frame receipts remain local; their hashes plus exact extracted CSV
and resource/identity summaries are committed instead of multi-megabyte dumps.

## Next accepted source gap

Close this LOD trial without another simplification/quality loop. Next bounded
independent action: inspect existing owned/licensed source material for the
accepted quiet **forest wind/foliage cue** (master 8.4/11); select one only if its
content/provenance fits, then reuse the existing finite incidental-voice budget.
This proposes no new mixer layer, renderer wind, paid asset or cue admission by
filename alone. The [remaining scope](../../design/HORDE_1_7_REMAINING_SCOPE_2026_10_20.md)
retains visual foliage wind/true LOD/combined-readiness gaps. Sam's six recovery
decisions remain in the unchanged morning handoff; mobile Mist Off remains selected.
