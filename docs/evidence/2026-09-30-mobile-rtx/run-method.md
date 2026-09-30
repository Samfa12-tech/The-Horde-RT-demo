# Native Mobile-dielectric RTX assessment method

Exact committed snapshot: `git archive f50c32fe2f9b5de8347f40b0c18399fe32b3a52f`,
unpacked to a new `source-f50c32f` directory. No worktree mutations or phone use.
This is the restored production shader, not the rejected first-blocker candidate.

Initial configure did not start CMake: plain `cmake` was not on PATH.
PowerShell command lookup failed (exit1); no CMake exit code or configure.log
exists for that attempt. This is a reconstruction from the worker's actual tool
output, not a regenerated raw log. The retained retry uses the VS2022 BuildTools
bundled CMake executable; configure-retry.log and build.log document success.

Configuration arguments: `-S source-f50c32f -B build-native-mobile`
`-G "Visual Studio 17 2022" -A x64`
`-DHORDE_RT_INSTRUMENTATION_OVERRIDE=Diagnostic`
`-DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=Mobile`.
Build arguments: `--build build-native-mobile --config Debug`
`--target horde_rt_diagnostic_window`.

Capture process arguments, with hidden native launch and bounded wait:

```text
--capture-showcase native-mobile-rtx-pipeline --development-checkpoint lantern-glass-production --capture-portrait
--capture-showcase native-mobile-rtx-compute --development-checkpoint lantern-glass-production --capture-portrait --require-rayquery-compute
```

The worker observed process IDs94012/93668 and exits0/0 in the tool transcript;
process codes were not saved in a raw sidecar. Main independently verified both
complete capture manifests, successful capture stdout, artifact hashes and
actual-module containment/validation receipt. Startup capability stdout occurs
before RT scene initialization and is not final presentation evidence.

Both native captures are540x960 at100%, with High WaterQuality imposed by the
existing capture path and **Mobile dielectric** modules selected through the
supported CMake override. Camera pitch resolves to-0.32 from requested-0.35.
This is RTX5050/Windows Diagnostic evidence only, not the phone's1080x2235,
Shipping performance, exact-device acceptance or the complete image/parity gate.
One capture per backend was the investigation cap; neither was repeated.

The lead's verify-paired-capture.ps1 checks artifact/cache/log/module/manifest/PNG
hashes and backend/presentation identity, compares both complete diagnostic
counter objects, and measures RGB deltas against the unchanged foundation pixel
limits. It intentionally reports a **pixel-only single-checkpoint subset**,
not a synthetic13-checkpoint foundation pass or timing acceptance.
