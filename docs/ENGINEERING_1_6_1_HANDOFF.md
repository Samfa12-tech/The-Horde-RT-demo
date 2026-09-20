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
The September 20 check reported 26% weekly remaining, so it did not trigger a pause.

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

1. Finish genuinely matched warmed 1.6.0/1.6.1 Release A/B, especially the phone
   baseline. Published 1.6.0 cannot run the new lantern preset; a backported test
   harness must be labelled as such, never as the unmodified public binary.
   Do not add another telemetry framework or hide the cost by lowering quality.
2. Then repair the four-way player semantic contract: processor/runtime GLB already
   emit Body/Head/NearFace/Gauntlet; the clip manifest and C++ semantic routing still
   describe only three. Make manifest/loader/texture mapping order-independent,
   reject missing/unknown/duplicate/conflicting semantics, regenerate cleanly and
   validate GCC/MSVC. Preserve primary masking of Head/NearFace.
3. Dedicated real-geometry RT viewmodel/world-body ownership remains unimplemented.
   Keep gameplay-owned IK/grip authority and independent small dynamic resources;
   no full-body primary-ray switch, overlays or permanent procedural block arms.
4. Physical glass correctness/performance remains open. Diagnostic held-high still
   records one overflow/pane-stack/primary-volume-budget event; secondary glass
   termination is also visible. Preserve these failures until actually fixed.
5. Complete justified memory/pacing/CI/docs, supplied adaptive Pocket Chordsmith
   music and separate volume, and Briarhold-derived reporting. Only then run the
   final comprehensive cross-platform/device/release matrix. No publication authorised.

## Recovery and device state

- Engineering checkout: `.worktrees/horde-1.6.1-engineering-pass` beneath the saved
  project. No active test run or worker remains from this checkpoint. The phone is
  returned Home; `.benchmark` contains the cleanup artifact. Stable/Debug apps and
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
