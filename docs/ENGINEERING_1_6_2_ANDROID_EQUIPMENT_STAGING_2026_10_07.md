# Android equipment staging — 7 October 2026

## Exact phone failure

Ordinary Play on immutable runtime `868691fc11e7ecf52b0e6ff99d7bab31e2b63997`
fails on the allocated SM-S948B / Android 16. APK SHA-256 is
`c865152bab855517fe48822c19fbda90bc242b4008e55b788c1f09d4d69ea93f`.
The Entry scene presents successfully, then Play requests the full Showcase.
Native initialization fails with `Could not read asset manifest` for
`models/props/runtime/player-rag-torch/asset.manifest.json` under the published
files root. The renderer safely returns to the Entry error screen; this is a
real gameplay handoff failure, not a Graphics persistence or surface-rotation
failure. The earlier affected menu/preview/rotation passes retain their limits.

Gradle and closed package admission include the Rag torch and scabbard, but
`MainActivity.collectInitialDiagnostics()` omitted their native file staging.
Both new runtime directories need staging before `writeReports(filesRoot)`
publishes the renderer root. The correction stages the exact five packaged
files and preserves the existing required-assets failure guard.

`Horde162AssetPolicyTests.ps1` now joins the exact Gradle equipment roster to
startup staging at the same native paths. The new positive assertion fails on
the original source for the missing Rag manifest before the implementation
changes. Negative fixtures remove each path or redirect its native destination.
Original failure log: `task-4/android-equipment-staging-before-fix-20261007.log`.
The corrected closed asset-policy run passes all 119 admission/staging/archive
cases (`task-4/android-equipment-staging-contract-20261007.log`); Android Debug
assembly also passes. Device retest requires a new immutable seal.

Private phone evidence is retained under
`task-4/integrated-162-868691fc-20261007/phone-pause-work`: fresh owned Entry/Play
UI, PID-scoped initialization logs and actual error screenshot SHA-256
`86da0efccb836595f28169e88b9f3292eadf23ec6355e7fa53c7e3a8723639c1`.
Owned PID 6335 is stopped. Every XML preference entry is identical before/after;
production app/data and owner font/density/portrait lock are unchanged.
Ordinary gameplay pause work was not measured because Play did not reach it.

## Desktop moving capture remains a gap

Three exact-runtime Windows `torch-low-opening` moving validations fail their
existing 30-second foreground-arm requirement. Owned PIDs 59264, 55696 and
37668 exit with code 1, zero captures and zero synchronization-validation errors.
The last attempt verifies a visible window belonging to its owned PID, requests
foreground once, and records Windows refusing activation. The foreground guard
was preserved. These attempts prove neither moving torch fit nor contact timing;
desktop foreground assistance is deferred until the owner returns.

The successful exact Windows Entry/Graphics/Play captures remain separate.
Internal review and automated evidence do not constitute Eric's independent
audit or owner motion/audio/touch acceptance. No release is published.
