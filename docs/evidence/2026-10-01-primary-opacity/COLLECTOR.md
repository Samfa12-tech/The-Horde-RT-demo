# S26 primary-opacity bounded trial

This folder contains a small adapter around the already-used Shipping trial collector/analyzer contract. `run-opacity-trial.ps1` is copied from `docs/evidence/2026-10-01-mobile-lantern-profile/run-trial.ps1`; its `Member` labels change to `normal-control` and `primary-opacity-candidate`. Lead review also adds one timed active RAM/system-memory/PSI snapshot at the first poll after30seconds, identically for every row; this is sparse RAM context, not GPU bandwidth or frame-aligned causal evidence. No game source, build, device action, benchmark, or image recapture was performed while preparing these scripts.

## Fixed row plan

Exact `SM-S948B` / `R5GL219SZGK`, `RayTracingPipeline`, Shipping/Mobile, 75%, current gameplay/music, no data clearing. Both treatments use the authored open-aperture geometry and OpaqueFast. ABBA order is A1 route + high-pose, B1 route + high-pose, B2 route + high-pose, A2 route + high-pose: eight total rows. Route denominator is 1,838 presented measured-lap rows, with its opening 160 reported separately; high-pose denominator is 600. Existing warmup exclusion, per-frame CPU/GPU completion joins, presentation denominator, package pullback, and lifecycle/memory sampling are retained. Do not repeat completed rows; incomplete row directories are preserved and halt further execution for review.

The normal-control APK is pinned to `3cb84efb2f71b1c96b8a562ae6a272569e3c76315053144582a617be75bf30eb` from source `6fa1c53d1f0e4ec3938983f2cad7bd2ece233f4a`; candidate APK is pinned to `b9d69ff43c13b0d84ff8fca11132578a27ae710ac2946546677d707655b9a188`. The candidate commit is mandatory input (`e78028c999e745e7cfd1695468062bb45f300b21` at preparation time) so stale candidate metadata cannot be silently reused.

The two actual package audits were checked read-only: all 70 assets are byte-identical; all four Mobile Shipping raygen modules differ as expected for the primary shader edit; both shared stages are identical; audited modules passed SPIR-V validation/disassembly with zero atomics and no diagnostic binding 22. The analyzer re-derives exact shader hashes from those audit files rather than hardcoding module hash expectations.

## Launch

Plan-only (no device calls):

```powershell
& 'C:\Dev\tmp\horde-primary-opacity-20261001\performance\run-opacity-matrix.ps1' -CandidateSourceCommit 'e78028c999e745e7cfd1695468062bb45f300b21'
```

Execute the bounded eight rows after the device and APKs are ready:

```powershell
& 'C:\Dev\tmp\horde-primary-opacity-20261001\performance\run-opacity-matrix.ps1' -CandidateSourceCommit 'e78028c999e745e7cfd1695468062bb45f300b21' -Execute
```

The adapter installs with replace/allow-test, preserves package data, pulls back and verifies the full installed APK hash before each workload, then delegates to the existing collector. To analyze after all rows complete and the two receipts exist:

```powershell
& 'C:\Dev\tmp\horde-primary-opacity-20261001\performance\analyse-opacity-matrix.ps1' -CandidateSourceCommit 'e78028c999e745e7cfd1695468062bb45f300b21'
```

Use `-ControlPackageAudit` / `-CandidatePackageAudit` only to supply the exact, independently generated audit receipts. The candidate default is `C:\Dev\tmp\horde-primary-opacity-20261001\audit-control-b9d69ff43c13\actual-apk-package-audit.json`; control default points to the restored-normal-source audit. The matrix analyzer writes one non-overwriting integrity report after validating all eight rows.

It reports native render-entry-through-present median/p95, owning command-buffer GPU median/p95, the separate opening-160 native/CPU-stage/GPU summaries, stage inventory, thermal/order context, and before/after RAM boundary snapshots. Shipping diagnostics are compiled out; GPU memory counters are unavailable, not a pass. Native-cycle median-derived FPS is not display-pacing proof; GPU duration includes AS build/update, RT, and copy, not isolated shader time. No performance acceptance, quality pass, display-FPS, bandwidth-saving, or optimization-admission conclusion is automated.
