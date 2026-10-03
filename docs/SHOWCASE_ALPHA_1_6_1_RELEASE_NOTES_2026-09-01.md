# Horde Lantern RT — Showcase Alpha 1.6.1

Package version: `1.6.1`

Android version code: `9`

## Status

Updated October 3. These notes describe1.6.1 implementation and accepted limits,
not publication authority. Exact artifact identity, completed signed S26
update/lifecycle checks and distribution status are recorded separately in the
[release validation](SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md).
The owner confirmed signing backup/recovery and authorised production signing/
freezing after green CI. Only verified distribution receipts establish public
availability. Acceptance below refers to retained exact builds; earlier owner/
device passes do not certify every subsequent candidate.

The previous1.6.0/versionCode8 artifact, device-smoke and release evidence remains
recorded separately. This update does not rewrite that history.

## Engineering changes

- Dedicated modelled hardware-RT viewmodel sleeves/arms/hands/gauntlets, with
  independent world-body/viewmodel geometry, dynamic buffers and BLAS, and shared
  gameplay animation/IK/grip authority. Normal production block arms are retired;
  diagnostic comparisons remain explicit. Player primitive/atlas/loader/skinning
  contracts agree; normal gameplay look-down and mirror presence are accepted.
  The latest bounded right-cuff repair is laptop-owner accepted; the owner also
  now confirms the phone cuff check green.
- Fixed Shipping/Diagnostic and Mobile/High shader variants, with separate
  opaque-fast/generic strategies and actual extracted Shipping SPIR-V checks
  showing no diagnostic atomics/readback binding. Pipeline and RayQueryCompute
  both traverse real Vulkan hardware BLAS/TLAS; no raster/fake-RT substitute.
- Mobile's reward lantern deliberately omits the glass panes. High/desktop
  retains the physical Fresnel/reflection/refraction/IOR/attenuation/TIR path and
  completed transport fixes. This is a quality-profile choice, not a model-name
  workaround or fake-transparency replacement. Remaining demonstrated High
  defects are owner-deferred future investigation, not fixed or passing.
- Adaptive A–H “What the Dark Keeps” score, with accepted whistle-lead fantasy
  instrumentation. Canonical PCS JSON/PCS1 remains revisable;48kHz stereo PCM16
  loop bodies/tails retain authored note/timing/frame-length contracts and the
  actual Chordsmith v68 app-voice/live-FX rendering route. Shared Pocket Audio
  Core owns reusable PCM playback; Horde owns gameplay cue decisions. No editor/
  synth application is copied into the game. Owner-supplied music is authorised
  for Horde use only, not generally permissively licensed.
- Removed only Melody3's held drone in sections A/D, preserving every other
  note/timing/arrangement. Canonical JSON/PCS1 and affected body/tail hashes agree;
  native loops/transitions and owner phone listening (“both perfect”) pass.
- Independent persisted Music and SFX volume controls, quieter stone footsteps,
  and the owner-accepted Windows gameplay-start/menu/refocus music repair.
- In-game reports use the approved Briarhold-derived Cloudflare relay/email
  architecture. Explicit consent and separate optional bounded typed diagnostics/
  game-only RT screenshot, preview, verification, cancellation and same-report
  retry preserve privacy; local JSON remains an offline fallback. Actual Android
  delivery and Windows preparation/preview/cancel are accepted. No client secrets,
  unrelated files/logs or second reporting service.
- Android serial off-UI scene startup/lifecycle ownership avoids the demonstrated
  resume ANR. The compatible render-scale path retains resolution-independent
  scene resources. New Android settings default to75%; saved choices persist.
- Dynamic coherent buffer lifetime mappings and compatible device-local/coherent
  immutable placement, preserving one frame in flight, fences, barriers and honest
  allocation failures. No FPS improvement is claimed from memory flags. Devices
  without a compatible local/coherent type retain required-host placement; generic
  staging there remains a future-platform gap.
- Current-source GCC/Clang/MSVC, Vulkan CPU-host player/resource fixtures,
  selected Clang ASan/UBSan and Android four-ABI/Java/lint CI. Driver reporting
  separates raw vendor driver identity from Vulkan API version. Interactive
  benchmark RT-loop FPS/ms is explicitly distinct from display Hz and disabled
  in unattended performance experiments.

## Validation and known limits

The [finding disposition](ENGINEERING_1_6_1_FINDING_STATUS.md)
and [finite integration matrix](evidence/2026-10-02-final-integration/README.md)
carry current evidence and next unfinished steps; do not restart completed tests.

- S26's finite resource/control and current-bank live/scale/lifecycle checks are
  complete. Heavy pair shows no meaningful gain; the current75% workloads remain
  above33.333ms. Owner confirms cuff, footsteps/SFX and music green and accepts
  measured performance as-is for1.6.1:30FPS is the target, not an achieved claim.
  Full graphics-options menu is planned for1.6.2; no further micro-optimisation.
  RAM pressure is separate from uncollected GPU bandwidth/cache/stall counters.
- Owner defers final S24 Shipping/live coverage: **working but not fully tested**.
  Prior accepted development images are not final-artifact certification; exact
  S25 is unverified. S26 or RTX results cannot certify either device.
- Foreground Windows Shipping/High Compute and interactive FPS/cancel/restart/
  completion pass on the current-player/music package,1838 complete presented
  CPU/GPU rows. Matched observer overhead is unmeasured, not a speedup claim;
  counter stays off in performance runs. Owner audio acceptance is not a new
  externally instrumented OS focus trace. No repeated audition/email is queued.
- Owner confirmed backups/recovery and authorised signing/freezing;
  exact signed S26 update/settings/RT/ASTC/Home-resume smoke now passes. S24/S25
  limits and publication authority remain separate. Actual1.6.0→public1.6.1 update dialog can only be
  observed after authorised GitHub publication; selection fixture passes.
  Compiler/mocked tests are not physical RT, sustained pacing or owner-feel proof.

Numerical sub-pixel parity and the remaining High glass defects are explicitly
[future work](../FUTURE_WORK.md#future-glass-investigation--owner-deferral-2026-10-03).
Failed evidence, unchanged tolerances and physical diagnostics are preserved.
Hotstrike redistribution remains the owner-tracked licence issue and was explicitly
declared nonblocking by the owner; no asset/licence/history/distribution change is
made here. Signing recovery and publication remain owner-controlled. The owner
authorised signing/packaging separately; these notes do not authorise merge or
publication.

## Implemented version contract

- Root `VERSION` is the sole semantic package/display-version source.
- Android version code is resolved from the checked `version-code-map.json`.
- Windows resources and Android package metadata are generated from that same
  active source identity.
