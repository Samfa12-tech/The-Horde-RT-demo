# Production-bank GPU stage probe admission

23 October 2026. Configuration/analyzer prerequisite only; **no new GPU-stage
profile, Android compilation or device success is claimed**.

The existing native timestamp queries already bracket AS recording, RT dispatch
and post-barrier transfer. They collect the successful submission's query results
after its owning fence/final successful idle. This change does not add renderer
work, shaders or a second submission identity.

`android/app/build.gradle` now permits explicit stage-only Shipping/Mobile Debug
chapter route timing without the private shader-root requirement. Defaults remain
OFF. Benchmark/staged/viewmodel combinations remain excluded; shader clocks still
require stages and the validated private root/ABI marker. No private bank is
substituted for production. Executable configuration positives/negatives are added.

The existing strict analyzers gain an explicit `--outdoor-hold` route for
`chapter-outdoor-scale-hold`, phase `lookout-sweep`, and diagnostic 20/33/40 cohorts.
Legacy defaults and their 33/40/50 rule remain. Selection uses each completed
submission's bound simulation-owner wall time in the declared 10-second warmup /
30-second sample. Every warmup and final stage row still joins exact tick/serial;
missing/unavailable/error values reject. A new query-test assertion disallows
`VK_QUERY_RESULT_WAIT_BIT`.

## Actual validation

- Rebuilt Release GPU frame timer collection tests pass.
- 18 strict motion tests and 7 stage-profile tests pass. The initial legacy error
  text mismatch is retained; implementation restores that text, not a weaker test.
- A saved real 834-frame control report, SHA-256
  `595afec1b587e3f504d036347a65ff042317d632d0a9d6794c3ec9a757f177c7`,
  validates only with explicit outdoor admission. Its old CSV correctly rejects
  stage profiling because no GPU-stage columns were collected. No old report is
  relabelled as a fresh profile.
- Existing native CMake policy fixtures pass with the authorized local temp path.
  The first sandbox-temp compiler failure is retained as an environment result.
- **Gradle policy cases remain blocked before configuration.** Its wrapper cannot
  open the existing normal cache lock at
  `C:\Users\sam_s\.gradle\wrapper\dists\gradle-8.11.1-all\2qik7nd48slq1ooc2496ixf4i\gradle-8.11.1-all.zip.lck`
  (`java.io.FileNotFoundException: Access is denied`). This is a filesystem denial,
  not a stage-policy rejection or automatic approval review. No retry/alternate
  cache/security change occurred. Actual Gradle positives/negatives and Android
  compilation are unverified; permission for normal existing-cache access is the
  next required action.

Exact commands and retained excerpts are in local ignored
`reports/production-stage-admission-20261023/receipt.json`; excerpts are labelled
where full logs survive only in completed tool output. No device operation,
installation or trace was performed. RT timers are execution intervals with
dependencies, not additive hardware counters or scanout FPS.

After cache access is resolved, run the configuration cases and build the actual
production-bank probe before proposing one fixed-resolution phone hold. Preserve
installed APKs, saves/settings, workload and natural thermal context. Do not loop
on cooling, use a mismatched APK as causal control, or expand tracing permission.
