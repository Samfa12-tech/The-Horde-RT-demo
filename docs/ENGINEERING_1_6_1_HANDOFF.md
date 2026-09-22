# 1.6.1 recovery handoff

Updated 2026-09-23. Branch: `codex/horde-1.6.1-engineering-pass`.
Engineering work has resumed by explicit owner instruction; the goal is not complete. See [programme scope](ENGINEERING_1_6_1_PLAN.md)
and [latest lantern evidence](ENGINEERING_1_6_1_LANTERN_BENCHMARK_2026-09-20.md).

## Owner's quota and backup instruction

The owner explicitly removed the approximately 5% usage guard and resumed work
on September 20. The `horde-usage-pause-guard` automation is confirmed absent.
Do not restore either the quota pause or explicit-resume restriction from older
handoffs. Preserve the full scope, selected model and effort, and normal service
limits; do not redeem credits or publish without authorization. Keep GitHub current
after coherent reviewed slices, preserving unrelated files and verifying remote SHA.

### Latest owner steering for continued work

Restore current CI early: inspect/reconcile PR #15 conflicts non-destructively,
preserve accepted implementation/guidance, add scoped branch-push validation,
obtain fresh current-source/integration results, and update the PR description.
Ensure actual skinned smoke/semantic fixtures are covered despite the portable
workflow disabling Vulkan targets. Do not merge into main, force-push or rewrite
history. Finish Phase 2 admission and affected RT images without restarting
Phase 1 or endlessly chasing historical parallel-export ordering. Define a
separate arms-only contract for the dedicated viewmodel. Keep Shipping/Diagnostic
parity distinct from pipeline/compute parity; investigate backend pixel divergence
without loosening tolerance. Include live motion, not just frozen extreme poses.
Music, consent-based reporting, resource work and final gates remain in scope;
licensing, signing recovery and publication remain owner-controlled.

### Current Phase 3 checkpoint

`6b219832514b3e36bda3792ad6639119894675c2` is pushed and remote-verified.
Fresh branch/PR CI `35799096847` / `35799100216` passes 44/44 portable and 10/10
Vulkan CPU-host tests; PR #15 is MERGEABLE/CLEAN. This handoff-only follow-up
changes no runtime. New commits are `34b27b4` (non-no-op stale-hash fixture),
`d8b1b7f` (native GPU geometry ownership), and `6b21983` (current shader/checkpoint
validation, including fresh failure-driven repairs). Earlier CPU/asset slices
`6404976`, `6a93eb7`, `fff4649`, `051fcde` remain accepted foundations.

World/viewmodel now have independent vertex buffers, BLAS/scratch ownership,
shared solved gameplay/IK/grip pose, named geometry roles, shared texture domains,
and fixed bindings 23/24. Slot 20 is a new primary-only instance; normal gameplay
still uses block arms. See [Phase 3 record](ENGINEERING_1_6_1_VIEWMODEL_2026-09-23.md)
and [nine native captures](evidence/2026-09-23-viewmodel-rt/README.md).

The candidate is under `assets/models/player/viewmodel/`; its SHA-256 is
`1e3b041ee7aa896a84fe462c182f6012d026b4a577b2d4ddf59d764b823f2538`.
It is rendered only in opt-in Windows development checkpoints and remains outside
Android packaging. The unchanged Phase 2 world-image tolerance passes, 13/13 MSVC
focused checks pass (plus the final inventory regression), both sets of eight
SPIR-V modules validate, and unsigned Android Shipping/Mobile builds all four
ABIs. Broader Windows non-Vulkan validation passed 56/59 initially, then all three
failed tests passed focused repair reruns; do not call that one clean 59/59 run.
The missing-optional-asset native check also passes: world-body presentation works,
and explicit viewmodel requests fail clearly without a full-body fallback.
Severe sleeve/shoulder presentation, live motion, retraction, exact phone and owner
acceptance remain open. One raised-lantern transport overflow is preserved as an
open finding, not hidden by capture success. Next: refine the model/skin presentation,
package and validate the candidate, and then obtain owner phone acceptance before
retiring block arms. Static sleeves have bounded indices/55 mm maximum bind-pose
edges; inspect exact posed CPU geometry next to distinguish deformation from
GPU decoding. That is a hypothesis, not a diagnosed/fixed art defect. Do not redo
Phase 2 or restore a quota pause. Audio/haptic manual revalidation required: NO
for this slice; feedback semantics/playback did not change. No phone installation,
paid generation, signing/licence change or publication occurred.

### Historical quota checkpoint (superseded status)

Engineering work through `614a0e2daea00f4eec3ce02792cbc1af3317ec5c` is pushed and
remote-verified; this final handoff-only checkpoint follows it on the same branch.
Measurement-only baseline `186068a4adcb223d3096eee779fcf2b607117503` is also
remote-verified and clean. Phase 2 commits since the Phase 1 gate:

- `4063f90`: shared typed four-way contract (3/3 targeted MSVC tests).
- `d26f1c9`: manifest/loader enforcement (5/5 targeted MSVC tests).
- `ef8fdc0`: canonical texture groups (4/4 targeted MSVC tests; four-ABI Android build).
- `81908e1`: actual skinned malformed/reordered fixtures (final 2/2 MSVC tests).
- `614a0e2`: reproducible single-thread export runner, evidence and metadata reconciliation.

No worker/build/benchmark remains active. No phone action, release publication,
signing change, source-asset replacement or licence change occurred in this slice.
Four pre-existing untracked scratch paths remain deliberately uncommitted. Curated
evidence and reproducible sources are backed up; local APKs/build caches/generated
validation GLBs are not a full workstation backup. Fresh GCC/CI, generated-runtime
admission and affected RT image/device checks remain open; do not mark Phase 2 green.

## Accepted work and current boundaries

- Native hardware RayQuery compatibility backend is implemented alongside the
  preferred RT pipeline. Exact S24/S25 acceptance is still open; S26 does not
  certify those devices.
- Completion-owned CPU/GPU evidence, Shipping/Diagnostic and Mobile/High variants,
  full-route reports, Release-safe automation and isolated development-signed
  Android benchmark packaging are implemented.
- Five additional lantern workloads now cover four frozen production-geometry
  views and the live reveal sequence. The historical route never claimed the
  lantern. Shared, Windows and Android commits: `6335049`, `efb04d1`, `7ccb753`.
- Windows Release completed all five cases; 5/5 focused host tests and 11 Android
  benchmark tests passed. Fresh Windows High shader parity passes 15 image pairs,
  including held glass, with existing RGB tolerance 3. This is Debug-host image
  evidence, not Release timing or Mobile-driver equivalence.
- Exact SM-S948B Shipping/Mobile artifact `0f30a7…d80a95` completed all five
  600-frame measurements at unchanged 100%/1440x2980. Medians were 100.86–146.95 ms.
  These expose the expensive workload; they are not an optimization result.
  Full hashes, conditions and raw report copies are linked in the evidence guide.
- Home interruption rejected stale reports but exposed temporary scene retention.
  `6576950` restores gameplay in the owning cancellation path. Cleanup artifact
  `faf42c…fc0eb1` was byte-matched on device, interrupted, relaunched and visually
  checked at the normal spawn/torch scene. Do not assign the earlier timings to it.
- Latest implementation/documentation head before this recovery checkpoint:
  `89b20bfcd9709e26a83e30c78bee0b57b8ef3628`, pushed and remote-verified.

## Next work

September 20 follow-on: [fresh Android route A/B](ENGINEERING_1_6_1_ANDROID_RELEASE_ABBA_2026-09-20.md)
is complete at matched 76% settings, with whole-run thermal context and a new
clean-source candidate `167ce8b` / APK `02112a…e24bc`. All four 1,838-frame runs
completed; −0.37% mean-of-run-medians is not a demonstrated gain given thermal
drift. The 15-file [recovery bundle](evidence/2026-09-20-android-route/README.md)
is tracked. `.benchmark` now contains that clean candidate, restored to 100%;
stable remains untouched at 76%, phone Home, no active sampler/benchmark.
Quota check after subsequent lantern measurement: 12% remaining; no pause threshold reached.

The narrow source-baseline harness is now implemented and pushed through
**`186068a`** on `codex/horde-160-lantern-baseline`. Worktree:
`C:\Dev\tmp\horde-160-lantern-baseline`; host build:
`C:\Dev\tmp\horde-160-harness-build`. Core2/2 MSVC tests, four-ABI Android build,
three Java policy tests and exact old shader/51-asset package proof passed.
It does not change the old renderer, shader bytes, assets or gameplay implementation.
Do not merge the old-source branch into engineering or publish its test package.

[Held-high source-baseline A/B](ENGINEERING_1_6_1_LANTERN_ABBA_2026-09-20.md)
completed at unchanged100%: A134.387/B130.6903/B130.5626/A134.578ms. The observed
−2.867% mean-of-run-medians difference is small, not a material glass optimization.
The checked scene-region pixels match exactly. Full report/hash/thermal limitations
are retained; both originals needed to reproduce pixel comparison are Git-LFS-backed.
This plus the preceding variant/counter/runtime/route evidence closes the Phase1
measurement-foundation gate; its explicit decision is in the report. Broader
glass correctness/performance, exact S24/S25 and final release gates remain open.

Installed `.baseline` is **`84104e3` / APK `5f9253…c4fee2`**. New live-reveal
runner alignment at `186068a` built as APK `4cbd08…5a3461`, but is **not installed**.
No timings belong to the later artifact. All four accepted held-high runs used
fresh-process `am start -S -W`; without `-S` one attempt only brought a task forward
and was excluded. Baseline/candidate remain100%, stable76%; phone Home, no sampler,
build, benchmark or worker remains active.

1. **Phase 2 is technically accepted at `56cd539`:** four-way manifests/processor/
   loaders/atlas agree; actual static/skinned addressing is checked before upload;
   reproducible `e8737f10…450fd` runtime is admitted. Five paired native RTX pose
   images are byte-identical. Fresh push/PR CI each pass 43/43 portable plus 8/8
   Vulkan-host player tests; MSVC and four-ABI Android packaging also pass. See
   [phase gate](ENGINEERING_1_6_1_PLAYER_CONTRACT_2026-09-20.md) and
   [image evidence](evidence/2026-09-23-player-admission/README.md). No new phone or
   subjective arm acceptance is claimed. Main documentation conflicts were resolved
   in `d3a7225`, current-source CI restored, and PR #15 is clean/mergeable.
2. **Phase 3 in progress:** the reproducible arms asset, explicit two-part role,
   and shared solved-pose CPU seam are implemented. Dedicated GPU viewmodel/world-body
   ownership and native/owner acceptance remain open.
   Keep gameplay-owned IK/grip authority and independent small dynamic resources;
   no full-body primary-ray switch, overlays or permanent procedural block arms.
3. Physical glass correctness/performance remains open. Diagnostic held-high still
   records one overflow/pane-stack/primary-volume-budget event; secondary glass
   termination is also visible. Preserve these failures until actually fixed.
   Expand genuinely matched measurement to remaining views/live reveal before
   accepting later optimizations; no more telemetry framework is needed.
4. Complete justified memory/pacing/CI/docs, supplied adaptive Pocket Chordsmith
   music and separate volume, and Briarhold-derived reporting. Only then run the
   final comprehensive cross-platform/device/release matrix. No publication authorised.

## Recovery and device state

- Engineering checkout: `.worktrees/horde-1.6.1-engineering-pass` beneath the saved
  project. No active test run or worker remains from this checkpoint. The phone is
  returned Home; `.benchmark` contains the clean `167ce8b` artifact described above.
  Stable/Debug apps and
  their data were not replaced or cleared. Phone permission is already granted;
  do not repeat Briarhold coordination merely because context was compacted.
- Curated report evidence is now tracked in [the recovery bundle](evidence/2026-09-20-lantern/README.md).
  Compiled binaries/APKs, raw PNGs, build caches and unreviewed scratch are still
  local-only; GitHub is not a full workstation backup. Rebuild/recapture from source
  where necessary, preserving the distinction from the original exact evidence.
- Local `.superpowers/.../progress.md` contains extended implementation history.
  Four unrelated untracked scratch paths remain; do not blindly stage or delete them.
- A delegated path-quoting mistake overwrote two old capture files and created one
  under `C:\Users\sam_s\Documents\the`. It was disclosed; no unverified restoration
  or deletion was attempted. Details are in the lantern evidence document.
- Hotstrike licensing/distribution and owner signing recovery remain owner-controlled.
