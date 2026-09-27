# Normal first-person look is the Phase3 acceptance boundary

The owner's2026-09-27 clarification supersedes the extended-camera study:
normal gameplay should show convincing torso/waist/legs/body presence when
looking substantially down, without duplicated arms, head/face/interior exposure,
wrist/armpit holes, popping, pitch-specific tricks or broken grips/RT transport.
No literal -90-degree requirement. Any later small adjustment needs an explicit
normal-play justification; do not redesign the player around extreme captures.

## Implemented correction

- Restore the established [-.32,.28] pitch-parameter range for both profiles in
  Android touch/JNI, Windows mouse/controller, simulation and shader camera bob.
  This parameter is not an Euler angle.
- Remove active extreme fixtures147/148 and the camera-only profile helper,
  BuildConfig flag and byte68 push-constant repurposing. Shared anatomical mounting,
  held poses/IK/grips, body remainder ownership and upper-torso partition remain.
- Preserve finite/clamped simulation construction and add regression checks for
  both profiles, live input and reset. Former extreme fixture IDs reject.
- Regenerate every shader module. Source, frozen catalogs and all embedded
  pipeline/compute/compatibility bytes match the pre-extension66f1149 baseline.
  No shader budget, diagnostic rule or image tolerance is relaxed.

Six focused Windows host tests pass; Windows and ARM64 Debug builds pass.
Fresh native normal-range141/145/146/144 captures pass. Low/down145 remains
exact PNG `93601b600892f1be8e666446a75bb2345bc693a19b76e6e96aca2796fb442e1f`.
These are bounded regression checks, not complete player acceptance.

Prepared corrected APK:
`C:/Dev/tmp/horde-normal-look-android-20260927-a/HordeLanternRT-1.6.1-viewmodel-debug-arm64.apk`,
SHA256 `c68fa94e75be1827dd8a1fb87ae83ab00f77ebef2b215ea8dd7b9aaad1ea3263`.
Actual packaged Mobile pipeline/compute SPIR-V containment/validation passes.
Installation/device results must be recorded separately before calling them passed.

Historical [upper-torso phone evidence](evidence/2026-09-27-upper-torso/phone/README.md)
remains labelled as an extreme-camera investigation. Its glass failures remain
real open findings, but its camera-inside-body poses are not visual acceptance gates.
Next finish normal-range wrist/armpit continuity, appropriate body presence and
continuous movement/combat/carry checks while preserving the owner's liked arms.

Audio/haptic manual revalidation required: **NO additional check for this camera
boundary correction**; established physical held transforms and event-time
listener/source semantics are unchanged. The earlier anatomical-profile check
remains open; this does not waive it. The1.6.1 goal remains active and incomplete.
