# Bounded Android RAM sidecar preparation

Fresh host result: **34 assertions PASS** under PowerShell7.6.6; fake ADB only.
Command: `pwsh -NoProfile -File tools/tests/Test-AndroidMemorySamples.ps1`.
No phone was contacted and no new phone memory/performance result is claimed.

`tools/capture-android-memory-samples.ps1` requires PowerShell7.5+ and a verified
runner `trial.json` identifying the exact source, APK, model, serial, package,
Shipping/Mobile policy, RayTracingPipeline and75% scale. Bounded command deadlines,
one live PID plus start-time checks before/after collection, refusal to overwrite,
raw outputs and partial receipts keep identity/availability failures explicit.

The host fixtures cover PSS/RSS/native/graphics separation, nullable partial fields,
system memory/PSI parsing, historical Debug-format parsing without relabelling,
single-line JSONL, operator-declared sample labels, sparse cadence, subprocess
timeout/error diagnostics, source gaps, and PID reuse during a source collection.
The guarded-script rerun also passed34 assertions after the PowerShell version
precondition was added. That was a changed-script check, not a repeated phone run.

RAM pressure is not GPU bandwidth. GPU counters remain `not-collected`; logical
record traffic is labelled analytical, not measured allocation or DRAM traffic.
On reconnection retain the exact counter roster and actual samples separately.
Use equal sampler cadence in matched warm trials; collection duration is observer
context, not a measured FPS impact. If it perturbs timing, keep profiled/unprofiled
runs separate. A complete receipt is not a pass for missing fields or counters.

Audio/haptic manual revalidation required: **NO**; no playback/feedback change.
