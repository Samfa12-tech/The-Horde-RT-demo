# 1.6.1 engineering programme and release gates

Updated 2026-09-20. Status: **in development; not ready for final comprehensive validation or release**.

This is the current next-step index for `codex/horde-1.6.1-engineering-pass`.
The owner-authorised full repository audit remains in scope. The 2026-09-13
update adds S24/S25 compatibility, adaptive music and player reporting, changes
the work order below, and defers the large validation matrix until the feature
set is complete. It does not authorise publication or relax any RT requirement.

## Authority and recovered state

- Original audit: `Horde_RT_Demo_1.6.0_Full_Repo_Code_Audit.md`, audit/main base
  `9572a874b12a7be0f5753a5c8754c4294b325e10`.
- Guidance reconciled against current main `bda1b99a62e1de883273dd21f267bcfc92bc5490`;
  [AGENTS.md](../AGENTS.md) is short, with task-scoped engine/validation references.
- Accepted engineering work through `191d799`: baseline/version recovery, compiled
  Shipping/Diagnostic and fixed Mobile/High shader policy, evidence contracts,
  renderer observation plumbing, and exact Debug phone evidence documentation.
  These are historical accepted gates, not a fresh pass for the current dirty tree.
- Frame-evidence integration was recovered, reviewed and accepted at `f443904`;
  [C2b evidence](ENGINEERING_1_6_1_FRAME_EVIDENCE_2026-09-13.md) preserves its exact
  build boundaries. Subsequent shared report/benchmark integration is listed below.
- Full-route evidence storage (`7287d57`), versioned report projection (`badcd23`)
  and Windows Release benchmark export/exit (`762cd35`) are implemented. A fresh
  staged Shipping/High Windows run retained all 1,838 CPU/GPU completions and
  exited0; [route evidence](ENGINEERING_1_6_1_ROUTE_EVIDENCE_2026-09-13.md) records
  its exact artifact and limitations. Android native route integration (`f46de7a`)
  passed a targeted four-ABI Debug build. Android unattended Release export
  (`2961ad0`, `e856161`) now has [native build and unit-test evidence](ANDROID_RELEASE_BENCHMARK_AUTOMATION_2026-09-20.md),
  and now has exact non-debuggable Shipping/Mobile phone export evidence for five
  new [lantern workloads](ENGINEERING_1_6_1_LANTERN_BENCHMARK_2026-09-20.md).
  The original route does not exercise the held lantern. Shared cases `6335049`,
  Windows `efb04d1` and Android `7ccb753` preserve that route while adding explicit
  frozen views and a live reveal sequence. [Clean Windows route A/B](ENGINEERING_1_6_1_CLEAN_SHIPPING_COMPARISON_2026-09-20.md)
  records short-run evidence, not a speedup. Fresh Windows High Shipping/Diagnostic
  shader parity passes 15 pairs including two held-lantern views; diagnostic glass
  counter failures remain visible and open. Sustained thermal-matched 1.6.0/1.6.1
  baseline A/B remains open; this does not close the original observability phase
  or final matrix. Exact Android Home-cancel cleanup was fixed/device-checked at
  `6576950` without replacing stable/Debug packages.
- Follow-on [exact Android Release route A/B](ENGINEERING_1_6_1_ANDROID_RELEASE_ABBA_2026-09-20.md)
  compares the byte-matched published APK with clean source `167ce8b`, both at
  76%/Mobile on SM-S948B. Four complete 1,838-frame reports and whole-run thermal
  context are GitHub-backed. The descriptive −0.37% aggregate is not a gain claim:
  thermal drift dominates. Source-baseline lantern harness core `513b5d3` is on
  separate branch `codex/horde-160-lantern-baseline`, reviewed and 2/2 focused
  MSVC-tested; its platform wiring, isolated packaging and matched held-lantern
  measurements remain next. Do not merge that old-source branch into development.
- Hardware RayQuery native integration was accepted at `99b8565`, after shader,
  provider and capability prerequisites. [Targeted backend evidence](ENGINEERING_1_6_1_RAYQUERY_BACKEND_2026-09-13.md)
  records Windows and exact SM-S948B functional/lifecycle checks. Exact S24/S25
  acceptance, cross-stage precision investigation and final-candidate matrix remain open.
- Implementation ledger and detailed audit briefs are retained locally under
  `.superpowers/sdd/Horde_RT_Demo_1.6.0_Full_Repo_Code_Audit/`; this tracked index
  carries the release scope rather than making that scratch history always-loaded.

## Required work order

| Order | Work | Exit evidence |
| --- | --- | --- |
| 1 | Reconcile lean agent guidance | Scoped diff/link/contract review; no runtime gate solely for prose |
| 2 | Review existing plans, device diagnostics and supplied music/reporting sources | Exact evidence inventory; explicit missing evidence and current-source reconciliation |
| 3 | Incorporate all additions in 1.6.1 plans | This plan, phase index and candidate notes agree; additions precede final validation |
| 4 | Resolve S24/S25 failures and reusable Android/Vulkan causes | Targeted regressions/build checks; precise capability diagnosis; exact-device acceptance remains explicit |
| 5 | Finish the original audit plus music/reporting below | Coherent implementation, small reviewed commits, affected tests and prepared artifacts |
| 6 | Final complete-candidate Windows/Android/device/release validation | Fresh complete matrix tied to exact source/artifacts, with required owner/device gates closed |
| 7 | Release decision | Explicit owner instruction; no automatic publish/sign/upload |

## Galaxy S24/S25 compatibility

The current-main screenshot-derived records identify S25 Ultra / Adreno 830 /
driver 512.800.64 and S24 Ultra / Adreno 750 / driver 512.762.41 as RayQuery-only:
AS/RQ/BDA/deferred-host-operations are exposed, but ray-tracing-pipeline is not.
The published bridge's pipeline-only launch gate prevents any scene attempt. No downstream
AS/shader/surface failure has been demonstrated. Exact model codes and APK hashes
are absent, and neither configuration has locally confirmed working RT presentation.

Bring the already planned alternate hardware `RayQueryCompute` backend in
[FUTURE_WORK.md](../FUTURE_WORK.md) into 1.6.1. Share real BLAS/TLAS ray traversal,
shading, resources and simulation with the preferred pipeline backend; only the
launcher, required features and synchronization/stage bindings differ. Preserve
the existing pipeline preference on S26/RTX. The main roadmap's former
post-1.6.1/music/compatibility order is superseded by the owner's current request.

Trace loader/capability enumeration, enabled features/extensions, queue/device
selection, BLAS/TLAS creation, shader/RT pipeline setup, surface/swapchain creation,
memory ownership and lifecycle/resume. Fix demonstrated reusable engine/platform
defects rather than model-name exceptions. Keep RT-only diagnostics honest when
hardware/driver capability is genuinely insufficient; no raster or fake-RT fallback.

Acceptance: targeted capability/feature-policy and failure-report tests, affected
Android native/package checks, and an exact candidate diagnostic/presentation/
lifecycle checklist for each affected variant. Report missing extensions/features
precisely. Do not certify a whole Galaxy family from the S26 Ultra or vendor claims.
The broad cross-device matrix runs against the final candidate, not every patch.

## Remaining original audit work

1. Finish fence/submission/epoch-owned observations and shared report/benchmark
   migration. Preserve non-fatal optional telemetry, diagnostic-free Shipping,
   fixed strategy identity, exact CPU/GPU labels, and every intended route sample.
   Add Release-safe benchmark start/export and graceful Windows exit support where
   the existing Debug-only harness cannot drive the actual Shipping path.
2. Repair player primitive semantics consistently across asset processor, manifest,
   loader, C++, textures and tests. Do not mask a mismatch with renderer constants.
3. Implement dedicated modelled native-RT `PlayerViewmodel` arms/sleeves/hands with
   a small independently owned dynamic buffer/BLAS/TLAS route, plus secondary-visible
   `PlayerWorldBody`. Use named instance semantics and the same gameplay-owned
   animation/IK/grip authority. Remove normal procedural block-arm presentation
   only after the replacement route is validated and owner-accepted on phone.
4. Measure and optimise reward-lantern glass on clean Shipping/Release matched A/B.
   Preserve Fresnel, reflection/refraction, IOR, attenuation, TIR and bounded real
   RT traversal. Investigation-only feature isolation must not ship. No unmeasured
   gain claim, silent resolution reduction or benchmark-specific lighting shortcut.
5. Complete justified static/dynamic memory ownership, pacing, naturally touched
   architecture extraction, cross-platform CI and documentation work. No gratuitous
   renderer rewrite or unrelated gameplay/enemy-capacity expansion.

## Adaptive score: What the Dark Keeps

Authority: supplied `What_the_Dark_Keeps_Horde_RT_Music_Pack.zip`, SHA-256
`e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa`.
Preserve its Pocket Chordsmith JSON and Markdown handoff; the preview MP3 is a
listening reference, not a runtime loop or the adaptive controller. Inspect current
Pocket Chordsmith/Pocket Audio locally before choosing native reuse or the documented
rendered-cue fallback. Do not replace the supplied composition or generate new music.

| Cue | Authoritative state | Musical behaviour at 80 BPM / 4/4 |
| --- | --- | --- |
| A | Exploration before actual torch failure | 12-second loop |
| B | Engaged, incomplete skeleton encounter | 12-second loop; first of two deaths does not end combat |
| C | Observed torch-extinguish event | 3-second one-shot, deduplicated per route/retry session |
| D | Dark exploration after torch failure | 12-second loop; checkpoint entry does not replay C |
| E | Active lich encounter | 12-second loop, continuous across charge/recovery |
| F | Defeated lich, reward waiting/raise/reveal | 12-second loop until actual roof-opening phase |
| G | Entry into `SkylightOpening` | Immediate 6-second one-shot; first F-sharp at 4.50 seconds |
| H | Dawn after G, or late completed-finale entry | 12-second loop; no stale darkness/combat override |

Use a shared testable state resolver consuming existing snapshots/events without
stealing SFX events or changing simulation timing. Use the audio clock for playback,
bounded crossfades/voices, snapshot reconstruction and event deduplication. Handle
pause, background/audio focus, death, retry/reset and late ending attachment.
Keep file IO/decoding/JSON parsing outside fixed ticks and audio callbacks.

Add separate persisted **Music Volume 0–100%** on Android and Windows, independent
of SFX. Preserve cue dynamics and exact loop periods/tails. Validate source import,
cue sample counts/loop metadata, all handoff resolver scenarios, twenty-loop drift/
seams, actual platform playback/lifecycle and packaging. Audio/haptic manual
revalidation required: **YES for music playback/mix**, not an invented requirement
to reapprove unrelated haptic code; final listening must protect warning/SFX clarity.

## Reusable player/playtest reporting

Inspect Briarhold's actual implementation before adapting it. Provide unobtrusive
Android/Windows report entry, useful player-authored issue text and appropriate
automatic build/device/renderer context. Share a bounded report schema and reusable
platform transport/service boundary instead of duplicating game-specific logic.

Preserve explicit user submission/consent, payload limits, redaction, actionable
failure feedback and safe retry/deduplication where applicable. Do not automatically
upload private logs, saves, credentials, identifiers or screenshots. No secrets in
clients. Backend deployment/account changes require explicit authority if needed;
prepare adapters/configuration and continue safe local work meanwhile.

Test schema/context/redaction/size limits, duplicate delivery, offline/failure paths,
cancellation, UI usability and Windows/Android adapter contracts. Real delivery is
separate from a mocked transport pass and must not send test reports unexpectedly.

## Final candidate gate and owner boundaries

Only after the complete feature set above is implemented, run the comprehensive
host/CI, Windows RTX presentation, Android build/package/device, shader/SPIR-V,
deterministic simulation/capture, asset/licence and release-readiness matrix.
Fix regressions and rerun affected checks; repeat full lanes only when necessary.
Bind all performance, presentation, lifecycle and listening claims to exact artifacts.

Required final handoff: per-audit-finding resolved/superseded/open/owner-device status;
branch/commits; Windows/Android/CI/SPIR-V results; S24/S25 cause and fix evidence;
viewmodel status and owner capture matrix; glass matched evidence; music/reporting
status; confirmed exact RT devices; remaining 1.6.1 work and final-pass readiness.

Hotstrike redistribution and signing backup/recovery remain owner-only. Do not
rewrite history, remove/replace the skeleton, change licences/distribution, spend
paid generation credits casually, publish, or declare absent device evidence green.
