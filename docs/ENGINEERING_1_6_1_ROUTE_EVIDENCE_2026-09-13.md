# 1.6.1 completed-frame route evidence

Status: C3 implementation in progress, not a performance or release acceptance.

## Accepted storage foundation

Commit `7287d57` adds a movable, bounded `RtBenchmarkEvidenceRun`. It allocates
three typed arrays only at explicit benchmark start, independently arms the final
measurement generation, and keeps every intended frame separate from accepted CPU
samples. Successful submissions bind actual coordinator identities; completions
join their saved lap/zone, including the final one-frame-late completion.

The existing replay fixture retains exactly 1,838 measured frames, with 599 in
the shadow corridor. Capacity is the authoritative 4,000-frame lap limit; overflow
invalidates rather than truncating a report. Successful device idle alone is not
completion: finalization also requires the expected ledger to be accounted for.

CPU and optional GPU availability are independent. Missing, pending, disabled,
unsupported and errored GPU samples are not zeros. Completed rows retain actual
presentation, CPU-stage and Diagnostic status. Successful SUBOPTIMAL presentation
is distinguished from failed presentation. Identity ordering is verified even for
CPU-ineligible frames.

Fresh targeted Debug and Release executable/CTest runs each passed (1/1 CTest).
Tests cover the production replay, final drain, all three allocation failures,
capacity overflow, moves, cancellation, malformed/stale/duplicate identities,
CPU/GPU independence, and retained failure causes. Independent review found and
verified fixes for completion-order bypass and lost failure metadata.

## Scope and limits

- One frame in flight remains binding. Out-of-submission-order multiple-flight
  collection was not introduced; it would require its own ownership design.
- A duplicate arriving after slot reuse is rejected as a mismatch; the run is
  invalidated rather than accepted. Finer error naming is deferred.
- Android route wiring and unattended Release export remain in progress.
- No new physical-device, matched-performance, remote-CI or final-candidate claim.
- Audio/haptic manual revalidation required: **NO**. This slice does not change
  gameplay events, listener/source data, transport, playback or haptics.

## September 20: shared reports and Windows Release integration

`badcd23` adds versioned completed-frame reports, with retained identities and
failure causes, separate CPU/GPU distributions and honest unavailable values.
The existing platform interval remains labelled separately; stage inverse duration
is not achieved presentation FPS. A regression test reproduced and fixed the case
where a complete two-frame subset could otherwise certify a 1,838-frame route.
Report completeness now also requires matching intended/legacy frame counts.

`762cd35` joins the real Windows submission/fence/final-idle path and adds
`--benchmark-showcase <output-directory>` in both Debug and Release. It invokes
the existing player route without checkpoint mutation or quality overrides,
writes both reports, and exits nonzero for incomplete evidence or failed writes.
Interactive startup diagnostics remain; unattended startup errors use stderr and
a nonzero exit without a blocking dialog. Independent source review accepted the
submission, final drain, cancellation and exit paths.

For a development Windows package with its assets beside the executable:

```powershell
$run = Start-Process -FilePath .\HordeLanternRT.exe `
    -ArgumentList '--benchmark-showcase', 'C:\HordeEvidence\run-001' `
    -Wait -PassThru
$run.ExitCode
```

Use a fresh output directory. The command retains the normal saved render scale
and quality; record the emitted settings rather than assuming them. This command
is not supported by the published1.6.0 executable. Capture/checkpoint automation
cannot be combined with it. It does not publish, update, sign or install anything.

Fresh targeted MSVC checks passed: Debug/Release 4/4 (route owner, report,
benchmark wrapper and command-line policy), followed by Debug 2/2 and Release
2/2 after the denominator fix. Windows Debug and Release linked. These are
targeted development gates, not the complete final-candidate matrix.

### Exact staged Shipping/High run

Retained at `reports/c3-route-shipping-staged-20260920/`:

- EXE SHA-256:
  `03c1245bc51d26a4155e52ea2b287c700fc6ce416841a5679e5a742f2388c921`.
- JSON report SHA-256:
  `7cb19173c30f5f3808bd56f61152a47d0cbbb0116bf672114901505bfcdf0008`.
- Source: worktree build from `7287d57` plus the shared-report/Windows changes
  subsequently committed as `badcd23` and `762cd35`; not a clean release package.
- Staged the same 29 runtime asset entries and two WAV collections listed by
  `tools/package-alpha.ps1`, plus existing licence inventory. No publication.
- RTX 5050 Laptop, native RayTracingPipeline, Shipping/High, 100% scale,
  1232x803 internal/presentation extent. No resolution reduction.
- Process exited **0**; both reports saved. Parsed schema2 JSON contains exactly
  1,838 intended/completed/CPU-accepted rows and 1,838 Valid GPU samples. No
  rejected, cancelled, outstanding or missing rows. Zone counts match the
  production replay, including 599 shadow-corridor frames.
- Every row has matching submitted/completed submission identity, increasing
  completion serial, successful presentation and `compiled-out` diagnostics.
  Final submission/completion3676 is retained at epoch2/generation6.
- Exact-EXE containment validated/disassembled all four expected modules: two
  pipeline and two hardware-RayQuery compute modules, each retaining real ray
  queries and containing **zero diagnostic atomics and no binding22**.

This run proves report accounting, Shipping selection and graceful process exit.
Concurrent development checks and the dirty build provenance preclude a matched
performance conclusion. No phone or cross-device acceptance is implied.

### Negative evidence

The initial Release test without packaged assets exposed a modal startup hang and
required termination (exit-1); it is explicitly not clean-exit evidence. After the
fix, the same missing-assets condition exited1 in under the20-second test bound
and retained the real missing-skeleton diagnostic. Evidence directories:
`reports/c3-route-release-20260920/` and
`reports/c3-route-missing-assets-20260920/`.

The September13 Debug legacy report exists, but its original process handle was
missing on resumption; no exit-status claim is made from that report alone.
