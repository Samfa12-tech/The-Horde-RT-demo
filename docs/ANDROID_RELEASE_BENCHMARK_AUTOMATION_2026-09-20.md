# Android Release-safe benchmark automation

Implemented by `2961ad0` and `e856161`. This is a development capability,
not certification of an installed APK or a completed matched A/B gate.

## Contract

An explicit `com.samfa12.hordelanternrt.action.BENCHMARK` Activity action with
`horde.benchmark.run_id` starts the existing in-app two-lap player benchmark once
RT is ready. The ID must match `[A-Za-z0-9_-]{1,64}` and be unique per run.
It is carried through JNI and render-thread ownership into both native reports.
It is not a new frame/submission counter.

This action works in Release without enabling Debug checkpoint controls. Mixing
Debug extras or the private forced-compute extra is rejected. It does not change
saved render scale, water quality or other render preferences. Use fresh-process,
matched settings for A/B work; inspect emitted settings rather than assuming them.
Automatic update prompts are suppressed during an automated run.

After native completion, export happens off the UI/render threads into:

`getExternalFilesDir(null)/benchmarks/<run-id>/`

- `benchmark.json`: exact native schema2 report.
- `benchmark.txt`: exact native text report.
- `result.json`: schema1 run ID, status and detail; committed last by rename.

A complete marker requires matching IDs in both payloads, native completion,
complete canonical evidence and matching nonzero intended/completed/CPU/row
counts with no rejected, cancelled or outstanding samples. Missing, stale,
malformed, oversized, fractional/overflowed-count or incomplete reports cannot
produce a complete marker. Invalid runs do not copy an old payload. Existing run
directories are never overwritten. JSON/text limits are16MiB/1MiB respectively.

Pause, Back, startup/runtime failure or the15-minute operational timeout produces
an invalid result rather than a performance pass. The Activity closes after export;
Android may retain the process in its cache. Process existence is not the completion
test. I/O failure is logged as `HORDE_BENCHMARK_EXPORT ... status=export-failed`;
a missing complete marker must be treated as failure.

## Future exact-artifact device check

These commands are preparation, not a request for owner action now. They require
an authorised, newly packaged candidate containing this implementation. Published
1.6.0 and older1.6.1 APKs do not implement it. No new APK was installed for this slice.

```powershell
$serial = 'VERIFIED_DEVICE_SERIAL'
$package = 'com.samfa12.hordelanternrt.debug' # Use the exact candidate package.
$runId = 'bench-' + [guid]::NewGuid().ToString('N')
adb -s $serial shell am start -S -W -n "$package/.MainActivity" `
    -a com.samfa12.hordelanternrt.action.BENCHMARK `
    --es horde.benchmark.run_id $runId
# After the export marker appears, pull this run only:
adb -s $serial pull "/sdcard/Android/data/$package/files/benchmarks/$runId" .
```

Before measuring, record the installed APK hash, exact model/driver, backend,
render settings, checkpoint order, temperature and thermal state. Inspect
`result.json`, the matching report IDs, all expected counts and the final owning
completion. Repeat Back/Home interruption as negative checks. Do not clear app
data, modify system font/render settings or substitute Debug timing for Release.

## Evidence and remaining gates

- Fresh MSVC shared benchmark report test passed after a missing-field RED.
- Android native Debug and Release builds passed all four ABIs. Release's CMake
  configuration is `RelWithDebInfo`; the existing Shipping/Mobile policy remains.
- Release ELF symbol inspection found the run-ID JNI export in all four libraries.
- Final focused Java tests passed in both Debug and Release: seven exporter tests
  and two intent-policy tests, zero failures/errors. The existing four contextual
  control tests also passed in the preceding Debug run.
- Read-only integration review checked run identity, stale-result rejection,
  lifecycle closure and the Release/Debug boundary.
- No APK install, physical lifecycle/storage pass, warmed A/B result, remote CI
  or final-candidate acceptance is claimed. Device storage access and Activity
  closure still require the targeted exact-APK check.

Audio/haptic manual revalidation required: **NO**. Normal gameplay/input/event
consumption is unchanged; this adds an explicit benchmark automation/export route.
