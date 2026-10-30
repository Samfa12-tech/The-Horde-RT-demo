# Android shadow selector correction

The remaining ordinary Graphics button used `(shadow + 1) % 3` after issue 23
retired Lower=0. Tapping Higher=2 therefore requested 0; the existing strict
admission threw `IllegalArgumentException: Invalid live graphics value`.

The production button now alternates the existing named Current/Higher values.
No shader, rendering quality, defaults, budget or graphics confirmation policy
changes. The new real `MainActivity` click test starts at Higher, checks four
successive wraparounds, preserves every other draft field, and verifies no apply,
preview request or persistence occurs. A second test opens the actual preview
popup, checks its two-item roster and stages Higher without applying/saving.

Selector audit: Android preview popup and controller modal both enumerate
`GraphicsPreviewOptions.choices(SHADOW)`, already `{Current,Higher}`. Android
`showRtLab` has scene/light/workload controls and no shadow-quality cycle.
Windows settings uses the existing `NextShadowQuality` Current/Higher helper.

## Reproduction and validation

From the existing active worktree's `android/` directory:

```powershell
.\gradlew.bat --offline --no-daemon --max-workers=2 testDebugUnitTest --tests com.samfa12.hordelanternrt.GraphicsPreviewOptionsTest.ordinaryGraphicsShadowCycleStaysCurrentOrHigherWithoutApplyingOrSaving --console=plain
.\gradlew.bat --offline --no-daemon --max-workers=2 testDebugUnitTest --tests com.samfa12.hordelanternrt.GraphicsPreviewOptionsTest --tests com.samfa12.hordelanternrt.GraphicsPreferencesTest --console=plain
```

Before the production fix, the new click test failed 1/1 at the actual button's
`withChoice` call. After the fix, 18 graphics preview/UI and 20 preference tests
passed: 38 total, zero failures, errors or skips. Gradle completed in 21 seconds.
Preferences coverage includes legacy Lower recovery. These are host Robolectric
SDK 34 tests with only the existing JNI boundary shadowed. Production Java was
compiled; no native Android build, APK install, device interaction, RT image,
touch feel or power/performance validation was performed by this slice.

The initial sandbox-only attempt could not create the existing Gradle wrapper
cache lock. The authorized local-test escalation ran successfully. A preliminary
log redirection used a missing report directory and did not start Gradle; that
path was corrected before the recorded reproduction.

Raw Gradle logs and baseline/fixed JUnit XML are retained locally in ignored
`reports/2026-10-18-mobile-shadow-cycle/`. The compact [receipt](receipt.json)
binds tested file hashes and observed results. Changes remain uncommitted for
the coordinating parent's integration checkpoint. A current device tap of
Current/Higher and preview return remains a separate acceptance check.
