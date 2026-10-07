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

## Corrected immutable phone checkpoint

Runtime `ecc16b82ac9044ff5f4b39e8be9723659d0e73cf`, tree
`e6bbfaa7570a66573ba615087f6050a3def51020`, is sealed separately under
`task-4/integrated-162-ecc16b82-20261007`. Four-ABI Debug APK SHA-256 is
`ccafc11b31ca51e5f6bdc4c6c4e18de14144eca45907cc0ebc8e0908c48e555f`,
138,462,724 bytes; installed base pullback matches. All four native payloads and
Windows executable bytes are unchanged from `868691fc`; the latter's exact
Entry/Play checks retain their earlier source/evidence identity. Both affected
Android assemblies and all 119 asset-policy cases pass. Runtime CI has all 12
aggregate checks successful, runs `37619496990` and `37619504576`.

Ordinary native Play now reaches the full RT Showcase on genuine Pipeline and
required RayQueryCompute at actual traced 720×1490 / output 1440×2980. Fresh UI
shows three hearts, accessible current/max health text and native combat actions;
the actual Pipeline gameplay image is SHA-256
`7ce5d83600c2957dedcb0f59146ea8f7ba92018f0522e9230afe72a8ab810bc1`.
Accepted completed frames use Showcase epoch 3. This closes the manifest/staging
handoff defect, without asserting a complete route or owner visual acceptance.

Both backends exercise ordinary Menu → Settings → Back → Resume → Menu and
portrait Home/resume. New surface generation 3 presents the paused Showcase;
later accepted completed frames use epoch 5. Pipeline retry/death also works.
The original idle gameplay interval reached death before the attempted Menu tap;
the fresh target guard correctly rejects that tap. Its CPU interval is not a
controlled active-gameplay baseline. Retry then immediate Menu supplies the
intended foreground pause test instead.

Actual native cadence aggregates establish bounded redundant-render suppression:
Pipeline has 48 steady approximately-five-second intervals with 9–10 render
attempts and 247–249 skipped iterations; Compute has 32 with 10 render attempts
and 248–249 skipped iterations. Death/menu/settings intervals are included, so
these are measured foreground-paused Showcase work counts, not live-preview or
background samples. Resume emits fallback-exit aggregates and accepts ordinary
UI input. Existing periodic capability files can retain a prior completed frame;
the duplicated settings readbacks are not used to prove new submissions.

Early Compute reads after Play and Home show actual presentation with no accepted
completed frame yet. Their original errors remain; later fresh cases 05 and 11
provide accepted completed-frame evidence. Pipeline case 16 captured the owned UI
and screenshot, then hit a missing private log-epoch input; corrected fresh case
17 supplies the complete evidence. None of these intermediate reads is relabelled
as a completed frame, persistence failure or build failure.

Every preference XML entry is unchanged before installation and after each block.
Original portrait lock, font 1.7 and density 560 are preserved. Owned PIDs 9419 and
14966 are stopped with receipts. Sustained FPS, reliable power/thermal reduction,
physical multi-finger feel, moving equipment/route activation, owner audio/haptic
acceptance and the final immutable review candidate remain separate gates.
