# Android benchmark validation build

The benchmark validation APK is an isolated, locally installable comparison
artifact. It is registered only when the Gradle property
`hordeBenchmarkValidation=true` is explicitly supplied:

```powershell
$env:HORDE_VALIDATION_UNSIGNED = '1'
.\gradlew.bat -PhordeBenchmarkValidation=true :app:assembleBenchmark --console=plain
```

The `benchmark` build type inherits the Release build, uses the fixed
Shipping/Mobile native policy with checkpoints disabled, is non-debuggable,
and receives the `.benchmark` application/package suffix. It is signed only by
the development debug key so it cannot replace the stable or Debug app and is
not a public release candidate. The benchmark opt-in also suppresses all
production signing-environment reads.

The resulting APK is under
`android/app/build/outputs/apk/benchmark/`. Use
`tests/AndroidBenchmarkValidationBuildTests.ps1` for the bounded manifest,
development-certificate, native-CMake-policy, and ARM64 package checks. This
build lane does not install an APK, inspect a physical device, or publish an
artifact.

Targeted September 20 validation passed: absent variant without opt-in; rejection
of Diagnostic/High overrides; isolated non-debuggable manifest and Android Debug
development certificate; exact benchmark build-model/cache checks for all four
ABIs (`RelWithDebInfo`, Shipping/Mobile, checkpoints OFF); and external SPIR-V
validation/disassembly plus byte-identical stripped/packaged ARM64 containment.
Evidence is `reports/android-benchmark-containment-20260920-145936.json`.
The tested APK was built with concurrent, uncommitted shared benchmark work and
is therefore WIP, not a clean-source candidate or physical-device pass. Rebuild
from the accepted source before using it for matched measurements.

Follow-on: clean detached source `167ce8b` was rebuilt through this same full
policy/package check, installed and pulled back byte-identically on SM-S948B.
See [exact Android route A/B](ENGINEERING_1_6_1_ANDROID_RELEASE_ABBA_2026-09-20.md)
for the new artifact and measured boundaries; this does not retrospectively
turn the initial WIP artifact into a clean build or certify lantern performance.

Audio/haptic manual revalidation required: **NO** for this packaging lane.
It does not change runtime events, playback or haptic behavior.

Automation may add the optional `horde.benchmark.workload` string extra to the
benchmark Intent. It is strictly allowlisted: `showcase-route-v1` (the default),
`lantern-held-high-v1`, `lantern-held-low-v1`, `lantern-grazing-v1`,
`lantern-motion-extreme-v1`, or `lantern-reveal-sequence-v1`. The selected
workload is carried through the native request and must match both exported
report markers; a wrong or malformed case cannot certify a run.
Lantern cases retain the same warm-up/measurement contract with 600 frames per
lap; the default showcase route remains the existing 1,838-frame measured case.
