# September 30 Shipping comparison protocol

Owner priority: retain correctness gates, establish achievable SM-S948B phone
performance at unchanged 75%, then make matched Shipping comparisons. No
resolution, quality, ray-budget, material, geometry or gameplay reduction.

## Artifact pair

- A: d504bfed51976e71e496ead81dbd284bec03da33 (03c), APK
  13a8676998c14fed139a14174757a5b1aecb48e50b902834799ab4c970875cf3.
- B: 8cbe0ac44d4fb3a168ca891985ca560b43fe4541 (runtime d63e29c), APK
  b5a4344b6f860259d88a4d7bae3d349f462704c9f5a689773626f8a93dc39c6f.
- Same ARM64 RelWithDebInfo/Shipping/Mobile, checkpoints OFF, non-debuggable
  testOnly development certificate and isolated `.benchmark` package.
- Same real native RT pipeline, ASTC asset semantics, player/animation/geometry,
  fixed benchmark workloads and measurement/export implementation. Fifty assets
  byte-identical; three text assets differ only in formatting (independent review).
- Install only `.benchmark`, preserve its settings/data and all stable/candidate
  apps. Retain exact installed APK pullback/hash for each swap. Do not force-stop
  during a trial, reuse IDs, record video, or relaunch on an observation timeout.

## Scope and order

Uncooled B0 route and B0 held-high are initial current-source observations, not
matched regression attribution. They completed before owner external cooling.
Owner reported an ice brick under the phone at approximately 06:49 UTC. Record
that changed condition: subsequent cooled results cannot certify ordinary
uncooled sustained gameplay. First A1 route spans cooldown and is exploratory
unless thermal state proves comparable. Keep every completed attempt, exclusions
and reasons. Do not select a faster subset or relabel a cooled run as sustained.

Use the existing 1,838-warm +1,838-measured ordinary route and 600-warm
+600-measured held-high/live-reveal workloads. Repeat both artifacts in reverse
order, with equal workloads/settings and post-launch battery/Android thermal/
GPU thermal-power observations (sampling starts after am-start observation,
not at exact warm-up/measurement boundaries). Prefer ABBA/BAAB; thermal overlap is necessary
but does not prove equal GPU clock (gpuclk is permission-denied). Report drift,
repeat spread and residual confounders rather than asserting causal significance.

Each admitted run requires all completion-owned rows, exact submitted/completed
identity and CPU index joins, valid GPU timestamps, real presented outcomes,
compiled-out diagnostic status/null readback, fixed shader hash equal to actual
packaged SPIR-V, zero ledger rejection/cancellation/failure, and correct extent.
Shader containment separately validates/disassembles the actual APK modules.
Shipping compiled-out counters do NOT certify physical glass correctness.

## Targets, not yet demonstrated achievements

- Ordinary gameplay: stable sustained 30 FPS first (33.333 ms budget), with
  recovery towards 45–60 only where unchanged-workload headroom supports it.
- Lantern-heavy: sustained 30 FPS minimum at the same 75%, not a scene-specific
  quality reduction. Current observation is far below this target.
- A median alone cannot certify the minimum. Report p95/tail/zone results;
  target at least 95% of render cycles within 33.333 ms for a 30 FPS budget,
  then verify actual display-present intervals separately. Native timings span
  render-entry through vkQueuePresent return, not displayed frame pacing.
- Cooled A/B can establish cost and upper-bound feasibility, not warm sustained
  acceptance. A final uncooled steady-state run remains required for any target
  claimed achievable in normal use. RTX/S24/S25/backend parity stay separate.

Existing glass failures/recovery admission, changed-pixel attribution, shadow and
live acceptance remain open. No new engine telemetry framework or fake glass.
Audio/haptic manual revalidation: NO; semantic cues/playback remain unchanged.
