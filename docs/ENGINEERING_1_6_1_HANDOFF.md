# 1.6.1 recovery handoff

Updated 2026-09-20. Branch: `codex/horde-1.6.1-engineering-pass`.
The full goal remains active, not complete. See [programme scope](ENGINEERING_1_6_1_PLAN.md)
and [latest lantern evidence](ENGINEERING_1_6_1_LANTERN_BENCHMARK_2026-09-20.md).

## Owner's quota and backup instruction

Pause this goal when the lowest **available** Codex usage window reaches about
5% remaining. Missing windows are not zero. Check usage at work boundaries as
well as the task's five-minute `horde-usage-pause-guard` heartbeat. Do not switch
models, redeem credits or shrink the goal. Before pausing, preserve a current
handoff and safely commit/push task-owned work without credentials, unrelated
files, force-push or release publication; verify the remote SHA. Keep GitHub
current after subsequent coherent, reviewed slices rather than waiting for 5%.
The latest September 20 check reported 7% weekly remaining, so it did not trigger a pause.

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

1. **Continue Phase 2:** the [typed contract and loader enforcement](ENGINEERING_1_6_1_PLAYER_CONTRACT_2026-09-20.md)
   now cover Body/Head/NearFace/Gauntlet, synchronize both manifests, and pass 5/5
   targeted MSVC Release tests. Static/skinned loaders enforce actual named parts;
   production player loading requires the declaration. The next texture-group
   slice passes 4/4 MSVC tests, including all 24 actual-player material orders and
   generated Body/Gauntlet atlas mapping. Next add skinned malformed fixtures,
   regenerate cleanly and validate GCC/MSVC/Android.
   Preserve primary masking of Head/NearFace. Phase 2 is not complete.
2. Dedicated real-geometry RT viewmodel/world-body ownership remains unimplemented.
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
