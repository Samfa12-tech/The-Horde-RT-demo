# Showcase Alpha 1.5.2 release validation - 2026-08-25

## Release identity

- Display: **Horde Lantern RT - Showcase Alpha 1.5.2**
- Package version: `1.5.2`
- Android package/version: `com.samfa12.hordelanternrt`, `versionCode 7`
- Release source: `f4891c48868e5511f2139dfd416848dcc9e658d3`
- Windows itch channel/build: `samfa12/the-horde:windows-x64`, `#1913191`
- Android itch channel/build: `samfa12/the-horde:android`, `#1913192`

Exact source, artifact hashes, package smoke, device boundary, itch build IDs, and public-channel verification are recorded below.

## Scope and evidence already accepted

- The RT Lab, general water-light transport, player/roof visibility, corrected waterfall-width axis, Windows scrolling/repaint behavior, and Android finale-overlay ownership are covered by the current source contracts and dated feature records.
- The owner accepted the corrected moving Windows water result, the waterfall width, and the Windows RT Lab scrolling/repaint behavior.
- The last pre-version Host run `reports/foundation-runs/run-20260825-070928/` passed shader freshness, 13/13 Debug and Release CTests, 13 deterministic Windows captures, Android Debug/unsigned Release/lint, package/licence checks, and evidence hashes. The fresh versioned release gate is recorded below.

## Fresh release-source validation

- Fresh versioned Host run `reports/foundation-runs/run-20260825-073343/` passed all seven stages on the exact candidate tree: shader freshness and negative safeguards, fresh Windows Debug and Release builds with 13/13 CTests in each configuration, 13 deterministic Windows captures, clean Android Debug and unsigned Release builds across all configured ABIs, `lintRelease`, package/licence checks, and evidence hashes.
- Raygen source SHA-256 is `99c09cb56cbd7411594138104bc042a628e727ee543effc80c20b09776012e11`; compiled SPIR-V SHA-256 is `157c82d8123adae6d14f80866dd2f3e66d4f2527753a04ae94810e06837af192`; the embedded word array matches exactly.
- The run recorded the pre-commit `a8088b6` HEAD plus the complete dirty candidate-file inventory before and after. No source file changed during validation. The clean release-source commit that captures that exact tree is recorded with the artifacts below.
- Final post-publication Host run `reports/foundation-runs/run-20260825-074907/` passed the same seven stages after the living-document reconciliation and immutable 1.5.2/versionCode-7 policy were applied. Its negative fixtures proved that 1.5.2 cannot be repackaged or re-uploaded and that the next Android release requires a code greater than 7.

## Exact artifacts and package smoke

| Artifact | SHA-256 | Bytes |
|---|---|---:|
| `Horde-Lantern-RT-Alpha-1.5.2-Windows-x64.zip` | `fd929f1972c4587c6720013eb0586934ab72924c5f8f9c50ec8576a23a57690d` | 67,207,375 |
| `Horde-Lantern-RT-Alpha-1.5.2-Android.apk` | `19593f9d8902052cb54f9b989f9646ec8cad97063db5d882e1487dd56a671182` | 59,778,679 |

- Signed packaging reran all 13 Windows Release CTests, Android Debug/signed Release builds, and `lintRelease` before producing the guarded candidates.
- Android signing retained established release-certificate SHA-256 `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`. `apksigner`, manifest identity `1.5.2` / code `7`, 16 KiB APK and ELF alignment, static C++ runtime, native libraries, credits/licences, waterfall asset, and release layout all passed. Signing values stayed outside Git and were cleared from process scope after packaging.
- The Windows ZIP was fresh-extracted to `reports/release-smoke/windows-1.5.2-20260825-074305/`. Its executable reported file/product version `1.5.2`, selected `RayTracingPipeline` on the NVIDIA GeForce RTX 5050 Laptop GPU, reported `RT scene presented: yes`, and exited normally with code 0 after `WM_CLOSE`. No game process was left running.

## Connected Android release smoke

`adb devices -l` exposed zero devices immediately before the release smoke. The exact signed APK was therefore not installed, launched, pulled back, lifecycle-tested, or observed selecting strict ASTC and honest RT presentation on a phone. No exact 1.5.2 signed-device, runtime, performance, or owner-feel claim is made.

The current water renderer has earlier exact `SM-S948B` Debug evidence in `WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md`, including repeated waterfall/skylight/lich timing, replay, captures, Home/resume, strict ASTC, and honest presentation. That evidence does not automatically validate the later overlay-ownership/width fixes or the signed 1.5.2 package. The signed 0.1.5 release remains the latest public APK directly installed and byte-matched on the phone.

## Itch publication

- Guarded preflight rechecked both candidate hashes, Android certificate, alignment, native runtime layout, and access to `samfa12/the-horde` before any upload.
- Butler activated Windows build `#1913191` and Android build `#1913192`, both at user version `1.5.2`.
- A fresh anonymous cache-busting fetch of `https://samfa12.itch.io/the-horde` returned HTTP 200 and exposed `the-horde-windows-x64.zip`, `Horde-Lantern-RT-Alpha-1.5.2-Android.apk`, and version `1.5.2`; the stale package version `0.1.5-alpha.1` was absent.
- The artifact channels were updated only. The authenticated itch description was not edited or saved; `release/SAMFA12_SITE_COPY.md` now contains the prepared 1.5.2 copy for a separately authorized page-edit action.

## Audio/haptic classification

`Audio/haptic manual revalidation required: NO` - the release changes RT lighting, bounded renderer tuning, UI ownership, and package identity only. Listener/source event-time data, spatialisation, playback backend, cues, event transport, feedback timing, and haptic routing are unchanged.

## Evidence boundary

Automated checks establish only their measured source, package, renderer, and lifecycle surfaces. They do not substitute for new owner feel, artistic judgment, audible-loop perception, or haptic perception. The owner-only signing backup checklist remains separate and unchecked.
