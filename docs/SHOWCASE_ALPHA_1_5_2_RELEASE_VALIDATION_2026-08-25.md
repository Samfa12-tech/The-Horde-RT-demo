# Showcase Alpha 1.5.2 release validation - 2026-08-25

## Release identity

- Display: **Horde Lantern RT - Showcase Alpha 1.5.2**
- Package version: `1.5.2`
- Android package/version: `com.samfa12.hordelanternrt`, `versionCode 7`
- Windows itch channel: `samfa12/the-horde:windows-x64`
- Android itch channel: `samfa12/the-horde:android`

Exact source, artifact hashes, package smoke, device evidence, itch build IDs, and public-channel verification are recorded below after the guarded release steps complete.

## Scope and evidence already accepted

- The RT Lab, general water-light transport, player/roof visibility, corrected waterfall-width axis, Windows scrolling/repaint behavior, and Android finale-overlay ownership are covered by the current source contracts and dated feature records.
- The owner accepted the corrected moving Windows water result, the waterfall width, and the Windows RT Lab scrolling/repaint behavior.
- The last pre-release Host run `reports/foundation-runs/run-20260825-070928/` passed shader freshness, 13/13 Debug and Release CTests, 13 deterministic Windows captures, Android Debug/unsigned Release/lint, package/licence checks, and evidence hashes. A fresh versioned release gate is required below.

## Fresh release-source validation

- Fresh versioned Host run `reports/foundation-runs/run-20260825-073343/` passed all seven stages on the exact candidate tree: shader freshness and negative safeguards, fresh Windows Debug and Release builds with 13/13 CTests in each configuration, 13 deterministic Windows captures, clean Android Debug and unsigned Release builds across all configured ABIs, `lintRelease`, package/licence checks, and evidence hashes.
- Raygen source SHA-256 is `99c09cb56cbd7411594138104bc042a628e727ee543effc80c20b09776012e11`; compiled SPIR-V SHA-256 is `157c82d8123adae6d14f80866dd2f3e66d4f2527753a04ae94810e06837af192`; the embedded word array matches exactly.
- The run recorded the pre-commit `a8088b6` HEAD plus the complete dirty candidate-file inventory before and after. No source file changed during validation. The clean release-source commit that captures that exact tree is recorded with the artifacts below.

## Exact artifacts and package smoke

Pending.

## Connected Android release smoke

Pending. Device evidence must identify the raw model code, exact installed APK bytes, established certificate, strict ASTC route, honest RT presentation, and lifecycle scope actually exercised.

## Itch publication

Pending. Both channels require guarded preflight, activation at user version `1.5.2`, and live-state verification.

## Audio/haptic classification

`Audio/haptic manual revalidation required: NO` - the release changes RT lighting, bounded renderer tuning, UI ownership, and package identity only. Listener/source event-time data, spatialisation, playback backend, cues, event transport, feedback timing, and haptic routing are unchanged.

## Evidence boundary

Automated checks establish only their measured source, package, renderer, and lifecycle surfaces. They do not substitute for new owner feel, artistic judgment, audible-loop perception, or haptic perception. The owner-only signing backup checklist remains separate and unchecked.
