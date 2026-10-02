# 1.6.1 engineering programme and release gates

Updated 2026-10-02. Status: **implementation substantially complete; release-critical acceptance remains open**.

This is the current next-step index for `codex/horde-1.6.1-engineering-pass`.
The owner-authorised full repository audit remains in scope. The 2026-09-13
update adds S24/S25 compatibility, adaptive music and player reporting, changes
the work order below, and defers the large validation matrix until the feature
set is complete. It does not authorise publication or relax any RT requirement.

Validated runtime checkpoint `00af842`: push36974679165 and PR36974684105
CI are SUCCESS. Modelled-player presentation, semantics, music bank/
playback, shader policy and Mobile open-aperture selection are accepted; no restart.
S24's discrete-definition TLAS refresh is integrated and restores hands/enemy hits
in actual Debug evidence. Exact Shipping/owner acceptance remains separate.
S26 report preparation/real game-only preview/READY lifecycle and live FPS-counter
update/cancel/reset pass. The one additional Android email is approved, sent once
and independently confirmed in the inbox. A subsequent rapid Home/resume ANR in
UI-thread Vulkan pipeline startup is now repaired with a serial off-UI owner;
the exact S26 interrupted-start and two rapid resumes pass without a new ANR.
The affected route now passes13/13 with four byte-identical control images and
honest resumed RT. Initial Host Debug92/97 stopped at five failures, now each
passing after bounded repairs. Release builds and96/97 tests finish; its new
strict-mode music staging failure is fixed with a targeted14/14 pass. Package,
Shipping SPIR-V containment and ARM64 object checks pass. The old packaged
Windows0-frame cancellation is retained, not relabelled a pass. Subsequent
Shipping173e1875 finishes Pipeline/explicit Compute route and live reveal,
exit0 with complete RT/GPU evidence. Background/acquire-tail limitations keep
foreground pacing/visual acceptance distinct. Debug RTX13 captures and Android
Debug/unsigned Release compile pass. Windows report prepare/preview/cancel,
bounded right-cuff repair, independent SFX balance and startup music focus are
owner-confirmed. No accepted player/audio tuning reopened.
Do not repeat accepted Android/Windows emails or completed focused lifecycle rows.

## Release-focused remaining gates — October2 owner direction

The owner directs that practical release/gameplay blockers take priority and
significant sub-pixel parity work stop unless it addresses a genuine physical
correctness defect. This section supersedes older "next pixel discriminator"
instructions, including the proposed held-high521,444 witness. It changes work
allocation/release triage, not ray transport, numerical tolerances or past results.
The full agreed feature scope is retained; no merge/publication is authorised.

| Priority | Remaining gate | Smallest useful completion evidence |
| --- | --- | --- |
| 1 — new laptop visual feedback | Guarded right-cuff pair is integrated in8b4433e and owner accepts laptop live movement. Paired native admission/protected-surface contracts pass; no other player tuning reopened. | Reuse the [finite cuff record](evidence/2026-10-02-right-cuff/README.md) and built changed APKs. Only change-triggered exact-device acceptance remains open. No repeated contrasts, broad arm rewrite or grip/IK adjustment. |
| 1 — real correctness and gameplay | Final production route must load/present, retain S24 hands/two-enemy visibility, accepted player/grips and stable lifecycle. World-box normal gates pass. The demonstrated UI-thread resume ANR is repaired: exact S26 pending cancellation/two rapid resumes and affected13-waypoint route pass with current RT. | Reuse the [completed surface-owner route](evidence/2026-10-02-s26-surface-session/README.md); finish [current-source integration](evidence/2026-10-02-final-integration/README.md) and final-candidate checks. Reuse the [normal correction evidence](evidence/2026-10-02-world-box-normals/README.md), not isolated UV/contact candidates. Do not whole-merge the experimental renderer or reopen accepted player tuning. |
| 1 — practical performance acceptance | Last warm Shipping/Mobile S26 cycle medians are81–86ms opening and112–123ms held/reveal at75%, not the33.3ms target. Pane removal helped greatly but did not establish comfortable30FPS. | Use existing results, then one final-candidate warm ordinary/lantern/live check with tails/thermal/pacing limits stated. Owner accepts actual playability or makes an explicit profile/product decision. No further unpromising micro-optimisation campaign. |
| 2 — finished reporting experience | Android hosted verification/one approved exchange and independent inbox note/context/PNG receipt PASS. Owner also confirms actual Windows prepare/preview/context/cancel and supplies the prepared-form screenshot; local JSON stays fallback. | Reuse [Android delivery](evidence/2026-10-02-s26-report-send/README.md) and [Windows owner acceptance](evidence/2026-10-02-windows-report-ui/README.md). Complete only current-source integration/selected-quality metadata contracts. No repeated UI check, second backend or extra email. |
| 2 — remaining change-triggered checks | Owner accepts current music and S24 screenshot appearance; Windows SFX balance/control and targeted Windows startup/menu/refocus repair5e20f10 are now accepted too. External-audio interruption/recovery and changed phone cuff/gain/final S24 live Shipping coverage remain separate. | Reuse these verdicts and the [Windows music finite record](evidence/2026-10-02-windows-music-startup/README.md). No repeated auditions or S24 screenshot acceptance question. Complete only distinct unfinished device/lifecycle rows. |
| 3 — one coherent final candidate | Current source/artifact CI/host/tooling, Vulkan/SPIR-V freshness and diagnostic-free Shipping, deterministic contracts, assets/packages, Windows RTX and exact Android presentation/lifecycle/resize/update path. | The finite integration matrix and four Shipping backend/live rows are completed with explicit limitations; reuse [backend receipts](evidence/2026-10-02-final-integration/windows-shipping-backends/README.md). Finish foreground/live Compute visual acceptance and exact-device rows; fix and rerun only invalidated checks. Include Shipping heavy-scene resize stress and bounded UI/FPS checks, not a new framework. Signing/publication require separate owner authority. |

### Physical defects versus numerical parity

- **Retain as genuine High correctness issues:** the demonstrated glass-exit/
  opaque-receiver contact ordering loses the actual receiver/creates a bright
  strip; the source-world row43 reference proves an invalid near-edge primary
  candidate, not merely different RGB arithmetic. Their [native contact evidence](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/646f508/docs/evidence/2026-10-02-backend-pixel-witness/axis-contact-policy.md)
  and [row43 record](evidence/2026-10-02-high-row43/README.md) stay open. The contact
  prototype is not production-admitted; its clean Shipping static gate fails.
  Only bounded work aimed at a demonstrated wrong path/visible artifact is
  justified. If no safe bounded fix is available, report the exact High limitation
  for owner disposition rather than silently waiving the physical gate or starting
  another expression/epsilon/precision/budget sweep. Mathematical corner perfection
  is not an unlimited engineering programme.
- **Park as diagnostic investigation, not release-blocking equivalence:** isolated
  RGB/UV-rounding/backend differences without demonstrated bad geometry/material/
  transport or gameplay impact. Preserve failed strict comparisons and unchanged
  tolerances; do not call them passed, physically explained or universal parity.
  Do not build the next521,444 observer solely because it differs. Ordered-UV
  precision and the failed staging/contact admission candidates stay isolated;
  no shader-cost gate is raised and no diagnostic is suppressed.
- **Already deliberately deferred:** physical Mobile lantern panes, further arm/
  directional-walk/feet/pitch polish and pane/model alignment. **Availability-bound:**
  S25's exact-device gate remains OPEN, not waived or certified from S24/S26;
  any future publication decision must acknowledge that gap and restrict claims.
  Unavailable GPU bandwidth/cache/stall counters and unmeasured counter overhead
  are evidence gaps, not gameplay defects. The counter remains out of matched runs.
- **Owner-controlled, not new engine work:** Hotstrike remains tracked and explicitly
  non-blocking by owner decision; preserve provenance/licence/distribution. Signing
  recovery and publication are not autonomously authorised.

Next engineering work is the production-only correctness/feature closeout and
final-candidate preparation above, **not** further parity instrumentation. Preserve
completed positive/negative evidence; no unchanged artifact rebuild or repeated
matrix without a specific validity change. Documentation-only reprioritisation
requires no phone install, shader compilation or manual audio/haptic revalidation.

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

Phase1 measurement-foundation gate is accepted after the
[exact held-high source-baseline A/B](ENGINEERING_1_6_1_LANTERN_ABBA_2026-09-20.md),
which records the gate-by-gate evidence and its public-APK/source-harness boundary.
Phase2 technical admission and Phase3 owner-accepted presentation are recorded
in the current [handoff](ENGINEERING_1_6_1_HANDOFF.md); do not restart them. Current
priority is Phase4 correctness and exact-phone Shipping performance, including
the [opening-room/ABBA evidence](evidence/2026-09-30-shipping-phone/README.md).
This is not closure of physical glass
correctness, all-view/causal performance, exact S24/S25 acceptance or final validation.
Historical entries above retain their original evidence status; the new gate
decision supersedes their then-open measurement-foundation status only.

October1 owner quality-profile decision deliberately supersedes physical lantern
panes as a1.6.1 **Mobile** requirement: omit the actual pane primitives for every
ray, retain native RT and unchanged cage/flame/lighting, and preserve full High
physical glass/assets/fixes for future Mobile work. This is not a fake-transparent
material or a phone-model exception. The [finite profile record](ENGINEERING_1_6_1_MOBILE_LANTERN_PROFILE_2026-10-01.md)
retains the completed24 exact S26/S24 Shipping comparisons, which show a large
heavy-workload saving but do **not** meet the sustained30FPS goal. Exact S24 hands/
enemy visibility, S26 live interaction/pacing, High physical-correctness/backend
gates and full final-candidate acceptance stay explicit. The owner reconnected S24;
the already-requested live visibility check must not be interrupted or resent.
Do not repeat finished experiments or reopen accepted player tuning.

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

Owner Phase3 acceptance update (2026-09-27): the current modelled candidate's
wrist, repaired inner-bicep/armpit, normal look-down angle and mirror appearance
are accepted. Directional walking is deferred to a later animation pass.
There are no noticeable look-down occlusion issues in owner playtesting. Visible
feet/steeper pitch are deliberately deferred until justified by future gameplay,
not1.6.1 exit criteria. Preserve the current range and accepted arm configuration;
finish coherent production integration and its affected validation without
reopening those accepted appearance decisions. Exact device evidence is in
`evidence/2026-09-27-segmented-seams/phone/README.md`.

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

## Deferred benchmark usability task (owner addition, 2026-09-30)

Add a live FPS counter while the benchmark runs, within this 1.6.1 goal but after
the current matched Shipping measurements and opening-room investigation. Reuse
existing timing/UI ownership; label the rolling sampling window and distinguish
native render-cycle FPS from display-presentation pacing. Keep the counter out
of the frozen A/B artifacts and avoid a new telemetry framework. Validate its
updates during live benchmark execution and its removal on exit/cancellation;
measure any observer overhead before using it in performance comparisons.

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

Hotstrike redistribution and signing backup/recovery remain owner-only. The owner
has contacted Hotstrike and tracks the unresolved permission issue on GitHub;
on October1 they explicitly state it does **not** block publication of this update.
Keep the issue open, provenance and licence statements unchanged, and do not
autonomously replace the asset or rewrite history/distribution. This is not new
licence permission or publication authorisation. Do not
rewrite history, remove/replace the skeleton, change licences/distribution, spend
paid generation credits casually, publish, or declare absent device evidence green.
