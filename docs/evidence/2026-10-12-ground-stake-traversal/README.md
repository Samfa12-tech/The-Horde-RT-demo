# Grounded stake and two-way rope checkpoint

Parent source: `901a603a2c25d9afe117982c981193e4f10a8ab4` on the existing PR27
branch. This is an integrated traversal milestone in the still-incomplete
tomb-to-woodland chapter, not a release or a completed streaming system.

## Implemented behaviour

The requested short iron stake replaces the overhead support. Eight shaft/rim
boxes, the lid and landing retain their existing interfaces. A small collar
connects the anchor to the stake. The new editable source, texture receipts and
native round trip are in [the source package](../../../assets/source/development-rescue-ground-stake-v1/README.md).

The physical line contacts the real apron and shaft. Ascent finishes with a
bounded planted stance; descent has its own lift, supported crossing, crouched
regrip and lowering sequence. Boots stay planted during the upper handoff.
Both hands use solved rope targets; sword and lantern remain stowed until safe
grounding and stance restoration. The physical lantern, its light, listener
height and camera follow the solved body offset. The camera still responds
independently of body turning.

An offset-approach regression was reproduced: the body could face away from the
line at first load, exceeding imported forearm reach. Grounded approach now
finishes the bounded body turn before gripping and reselects the actually solved
grips at commitment. A failed approach restores its original collision-validated
safe pose. Reconstruction republishes hand coordinates after preserving rope
particles; stale callbacks cannot commit a new generation.

Final review added the rendered tie collar to the actual skinned-vertex test and
reproduced glove penetration at Y=2.612 m. Its underside is now Y=2.64 m instead
of 2.61 m, retaining the anchor, topology and grip timing. The failing native
run is [preserved](collar-clearance-failed.log); both rig/adversarial lanes pass
with collar checks retained. Initial pre-correction PNGs/manifests and their
receipt remain here as before evidence.

Reversible parameters: planted drop 0..0.72 m, backset 0.25 times drop, drop change
0.015 m/tick, body turn 0.04 rad/tick and approach alignment 0.041 rad. Existing
12 nodes, 12 constraint passes, 60 Hz authority, 4.8 m rope, climb motor, 0.14 s
hand cadence, deployment pause/warning, gravity and free player-bump response
remain. Contact uses the 35 mm rope with 7 mm skin, rounded capsule ends and a
60 mm interpolated grip envelope. Loaded grip velocities receive 0.80 damping;
free-node damping is unchanged. No combat, control mapping or shader changes.

## Actual RT gallery

All eight PNGs were opened and inspected. These are the actual Debug RT storage
image at 960x540, 100% scale, High optical profile and HIGHER shadows on the
Windows NVIDIA RTX 5050 Laptop GPU. Each backend uses the identical fixed-step
snapshot and legal camera. See [corrected-capture-receipt.json](corrected-capture-receipt.json) and
each adjacent full manifest for exact camera, 12 rope particles, grip positions,
body pose, equipment transforms, masks, GPU completion, resource and shader IDs.
Victory/claim are seeded by the development helper; the rope then deploys and
traverses through real shared fixed steps. Repeat ascent first executes the full
ascent/descent round trip. This is not an owner combat playthrough.

| State | RayTracingPipeline | RayQueryCompute |
| --- | --- | --- |
| Loaded ascent | [Frame](ascent-pipeline-corrected.png) | [Frame](ascent-query-corrected.png) |
| Planted pull-up | [Frame](pullup-pipeline-corrected.png) | [Frame](pullup-query-corrected.png) |
| Supported descent regrip | [Frame](descent-pipeline-corrected.png) | [Frame](descent-query-corrected.png) |
| Second ascent after return | [Frame](repeat-ascent-pipeline-corrected.png) | [Frame](repeat-ascent-query-corrected.png) |

The inspected frames show no overhead apparatus; the shaft and line remain
continuous. Arms are present on both backends (58,165 / 8,675 / 1,647 / 142,803
primary pixels by state). The night-time hand views are dark, and pull-up/descent
are tightly framed. Those limitations remain visible; pixel counts do not prove
convincing grip animation or owner visual acceptance. The rear-mounted lantern
need not enter the primary camera while both hands grip the rope. Its instance
masks and authoritative ring attachment remain checked; a new strict snapshot
witness has 72 negative mutations and does not exempt arm visibility.

The four final short frozen captures per backend have last completed GPU RT times
3.511..7.132 ms and loop medians 6.669..10.622 ms. Initial pre-collar observations
were 2.756..3.604 ms and 6.072..8.066 ms. These uncontrolled short runs vary beyond
15%; they do not establish a collar cost or acceptable performance. Resource,
shader, camera and particle counts are retained, while the corrected collar
reveals 339/594 additional arm pixels at pull-up/descent. Controlled sustained
comparison remains a required measurement gate. The runs include cold-start outliers
and frozen simulation; these are **not sustained traversal measurements** or
phone performance. Actual inventory remains 20 BLAS, one TLAS, 28 TLAS instances,
52 buffers and 63 allocations, 43,192,528 host-visible bytes and 396,691,232
device-local bytes (classifications are not additive). All world/texture resources
remain shared and resident. Primitive owner metadata is not GPU streaming.

Selected Pipeline shader hashes are unchanged:
opaque `d56178b8af77f767db60233ad41788f838b11a81a36f77f8d924c8bd871ec352`,
generic `315ae69ae2548be21d7240f904b61cc1d1c1d492fefb835796cdffc309a664c7`.
Query identities are in the corresponding manifests. The frozen budget file is
unchanged: `56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf`.

## Artifact and validation receipts

The binaries below were built from the final runtime changes before this source
commit; subsequent edits were test/documentation only. They are development
artifacts, not signed release packages.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Windows Debug `HordeLanternRT.exe` | 12,547,072 | `9f00e19b32c26cedf752b5cf9eea20cb12295423ceae349191112aee16901f6e` |
| Windows Release `HordeLanternRT.exe` | 4,369,920 | `646329b77ba4887127f4597efd8af448cc05c805b7f7cdd0acdc3dcc3f4ff954` |
| Android Debug `app-debug.apk` | 172,758,411 | `ed4c9dd5e4d91df6dd850548a17b518470817679647c8b5804af6baff7d234ff` |

- Final Release: [20/20 affected CTests](release-final-tests.log), including all
  seven requested CI traversal checks, native geometry/rig, adversarial regression
  and the complete original torch sweep with explicit `--timeout 180` (15.57 s).
- Android: four-ABI Debug native/Java build, 284 tests in 43 suites, no failures,
  errors or skips; lint and exact runtime/held-item package checks pass. Logs:
  [Final build](android-final-build-tests.log), [roster](android-package.log),
  [held items](held-item-package.log). The final collar rebuild takes 50 seconds;
  unchanged Java/lint tasks reuse the successful 284-test analysis.
- Full initial Debug: 168/182 passed, 14 failed; failures are preserved in
  `reports/ground-stake-debug-tests.log`. The stale removed-anchor index, stale
  checkpoint-count witness and offset forearm-reach failures were corrected.
  MSBuild/glslang/asset-tool/permission failures from sandbox-mapped temporary
  directories were rerun unchanged outside the sandbox. The [29-test retry](debug-current-retry-tests.log)
  passes 28, with only the not-yet-rebuilt registry binary failing; the [rebuilt
  registry](debug-registry-closure.log) passes. After the collar fix, [all eight
  affected Debug checks](debug-collar-tests.log) pass, including native rig and
  both adversarial offset approaches. Thus all 29 selected checks pass across
  current affected reruns. The original torch was repeated in an otherwise idle
  lane with explicit `--timeout 180`: 171.73 s, pass. A previous concurrent Debug
  repeat took 180.32 s without that explicit command-line timeout; it is retained
  as an uncontrolled observation, not the deadline pass.
- No deadline, assertion or production rendering capacity was increased.

Earlier failed geometry/reach/contact/Release-capture attempts remain under
`reports/ground-stake-*`; Release correctly rejects Debug capture automation.
The original protected `tomb-dressing-v01.blend1` still hashes
`0dae1e8fb803336cef2d045f48c5f4c9195a6463f45161cb0b6c7434350769fb`.
This confirms preservation of its current bytes, not recovery of the earlier
historically overwritten backup.

## Remaining chapter and physical gates

Genuine two-way GPU preparation, ownership/descriptor transfer and fence-safe
retirement remain required. Graveyard/lid composition and remaining admitted
forest props also remain active chapter work. These captures supplement the
existing tomb/terrain/Kit evidence; they are not the final complete 360-degree
chapter gallery. Tree-generator/canopy expansion and mist issue28 remain deferred.

The new APK has **not** been installed or physically accepted. Owner Windows
traversal feel and fresh audio/haptic review are pending because the physical
listener now follows the planted stance. Android Current/Higher and saved
Lower→Current migration remain pending physical tests, alongside touch/lifecycle,
whole chapter, thermal/sustained RT/resource overlap and owner listening evidence.
PR27 base conflicts remain; no merge/main merge/history rewrite is authorized.

Next owner route: defeat Keeper → claim → watch stake-anchored deployment → bump
the free rope → ascend from centred and offset approaches → inspect hands/boots
and restored low lantern → fresh raise to Kit → trail/lookout → backtrack/descent
→ second ascent. Include pause/resume during pull-up and descent; report clipping,
snaps, hand readability, unexpected input, voice/light or equipment loss.
