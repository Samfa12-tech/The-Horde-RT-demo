# SM-S948B frozen arm-candidate baseline

Run `reports/android-showcase-runs/run-20260923-162801`, 2026-09-23.
This is **not arm acceptance**. The research steering freezes asset/roll changes
while solver, mounting, skinning/topology and body visibility are isolated.

- Exact APK and installed SHA-256:
  `6047b285d6507ce84ccc90b48faa52b07cc917446167dc271f5bc6ebd244b2f9`.
  This is the earlier candidate built from working changes based on `b352599`,
  not the harness's reported HEAD `362fc27` and not the new `69014cf` solver fix.
- Separate `com.samfa12.hordelanternrt.debug.viewmodel`; install required explicit
  `adb install -r -t` for this test-only artifact. No app data cleared; production
  app untouched. Existing harness ran with `-SkipBuild -SkipInstall -ApkPath`.
- Paired world `4050e15b5084bfa83ad9db09f8bb8ececd872be89a27d0b08447c12f9f7a501d`,
  view `194aab2874993c68b0ab5950ed82085564e8567b9a5f8126f27ceb11283f5e06`.
  Corrected Right source, frozen 105/145 test rolls; no sleeve caps.
- Raw model **SM-S948B**, Adreno 840, Android 16/API 36. Strict ASTC and hardware
  RayTracingPipeline; honest presentation, modelled route, selected 60 Hz skinning,
  actual skin updates, 21 instances and <=15 mm socket gate pass.
- Existing deterministic 13-waypoint route completes. All eight viewmodel
  checkpoints capture at 75%, 12 stable presented frames. Home/resume passes
  with a new honest RT presentation. No Shipping timing comparison was run.

## Failure retained and corrected at its layer

First run `run-20260923-161421` failed because the script assumed zero animation
time for viewmodel attacks (and retained older body-attack times). Current shared
staging intentionally freezes late active frames. Native staging assertions and
24 script positive/negative controls verify the exact values:

| Pose | Walk time | Combat time | Relative edges / swing events |
| --- | --- | --- | --- |
| Downward cut | .5833 | .4033 | 1 / 1 |
| Upward slice | .6167 | .1667 | 2 / 2 |

Both body/viewmodel routes now use these values with **1 ms** tolerance, exact
action and event checks. No image/grip/presentation gate is weakened. The second
physical run passes with zero harness warnings/failures; the first is not relabelled.

## Visual and transport gates remain open

Direct inspection of rest/downward-cut/look-down/raised-lantern PNGs confirms this
is not a satisfactory art candidate. Hands remain poorly framed, sleeve/foreground
surfaces need isolation, and looking down does not show the required torso/legs.
Do not attribute background gallery swatches to arms or hide geometry to pass.
The actual high/low lantern frames retain **4 / 2** transport volume-budget
overflows. Harness completion is not physical glass correctness.

Replay proves automated movement/state progression, not visible live-motion,
near-wall grip continuity, pickup/raise/lower transitions or owner's hand anatomy
approval. Those gates remain required. This S26 result does not certify S24/S25.
Audio/haptic manual revalidation required: **NO**; no feedback change in this slice.

All eight exact state records and the replay state are retained here, plus two
representative unedited phone PNGs (look-down and high lantern). The other six
full-resolution images and complete raw logs remain in the local run directory;
their hashes are in the capture manifest (only the device serial is omitted
from this public copy). This is a scoped evidence
checkpoint, not a full APK/build-cache backup. Reproduce using the existing
runner, same immutable APK, candidate directory and eight checkpoint names in
the Android candidate document; use explicit serial/model as that runner requires.
