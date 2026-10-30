# Android opening-menu Custom benchmark

Source basis: `6797e9d3`, with the exact source-file hashes and APK identity in `receipt.json`. This is an implemented and compiled ownership slice, **unadmitted for physical runtime acceptance**. It is not fixed-resolution/profile implementation or public ranking.

The opening menu's More page now offers **Custom benchmark (unranked)**. The ordinary active-game menu no longer offers a benchmark. Explicit unattended benchmark and motion-validation requests retain their separate guarded paths.

The JNI request carries a serial and originating surface generation. The render owner rechecks a genuinely presented Entry, no active/pending campaign, no Play/graphics handoff and no other capture/benchmark. It retains the same unselected campaign facade and suspends its pump during ownership. Entry retirement and an actually presented Showcase are required before the existing course starts. A course result alone cannot expose the menu: a fresh current Entry presentation and unchanged settings are required to release ownership. Surface loss makes cancellation sticky; failed retirement blocks instead of manufacturing restoration. The admission mutex also serializes competing campaign, Play, graphics and diagnostic deposits.

Checks actually run:

- Shared/Android ownership CTest: **63 checks, zero failures**.
- Focused Java: **six classes, 30 tests**, zero failures/errors/skips; includes actual Activity button publication, stale generations, Back/pause/surface cancellation, active-game exclusion and competing menu actions.
- Actual native Debug compilation for arm64-v8a, armeabi-v7a, x86 and x86_64, `assembleDebug` and `lintDebug`: **passed**, 99 s. Lint reports zero errors and 79 warnings; warnings are not suppressed or represented as a clean warning-free build.
- APK: 200596480 bytes, SHA256 `00a74c39115dd9b4874748598a2321f055b9e1e95bde368b1ee3b5c3c0e1f8c5`. It was **not installed** and is not a release artifact.

No phone, campaign save, persisted graphics, volume, rotation preference or report transmission was operated by this validation. Host policies, Robolectric and compilation cannot prove actual Vulkan retirement/presentation, Android lifecycle, touch/controller feel or audible transitions. Audio/haptic manual revalidation required: **YES**, for the newly exposed Entry/Showcase/Entry music transition; no previous forest listening approval accepts that transition.

## Retained blocked attempt

Automatic approval review rejected an earlier patch adding `android_opening_benchmark.inl` because it contained `gCampaignSession.reset()` and `gCampaignOwnerRoot.clear()`:

> This action was rejected due to unacceptable risk.
> Reason: The patch adds core bridge lifecycle logic that directly resets and clears the campaign session/owner state (`gCampaignSession.reset()`, `gCampaignOwnerRoot.clear()`), creating a significant risk of discarding campaign state and violating the explicit requirement never to discard active campaign data.
> Do not bypass this rejection through a workaround or indirect execution. Continue with a safer alternative, or carry out checks to prove that the action is authorized or low risk before trying again.

That patch did not execute; its absence was verified. The retained implementation contains neither operation and preserves the facade. The normal Gradle cache lock initially reported access denied; the once-authorized normal escalation using the **existing cache** succeeded. No alternate cache, lock deletion or permissions/security change was used.

## Still unfinished

Android provisional fixed output/internal dimensions, preserve-aspect presentation, temporary profile adoption and acknowledged rollback are still source work. The menu deliberately offers only Custom rather than pretending that requested settings are actual submitted dimensions. Physical Entry-to-course-to-Entry, Home/rotation/cancel and transition listening remain pending on the current build. Reports remain local/unranked.
