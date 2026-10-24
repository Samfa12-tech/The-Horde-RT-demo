# Android provisional fixed-output benchmark source

**Implemented and host/build tested; physical RT/profile acceptance remains open.** The opening More page now offers a separate Mobile provisional/unranked attempt alongside Custom. This is not ranked eligibility, scanout FPS or a leaderboard launch. No phone, installation or report transmission occurred.

The provisional specification remains the existing640x360 internal /1280x720 output Mobile profile, DRS off and its explicit graphics tuple. A supported attempt drains the current owner before UI buffer mutation, publishes request/origin/surface/sequence-tagged commands, then recreates that same owner after a matching holder acknowledgement. Measurement starts only after completed current RT evidence matches the exact profile, dimensions, aspect, backend/quality and effects. Requested pixels or Java callbacks alone cannot admit it.

The UI fits a centred16:9 SurfaceView within display space, with normal integer-pixel layout rounding, while status/HUD remains at display resolution. It restores the exact original layout policy using `setSizeFromLayout()`. Graphics overrides stay in native working state; confirmed/pending preferences, volume and orientation choices are untouched. Success, Back, failed apply, pause and surface replacement all require reverse restoration and a fresh Entry acknowledgement before terminal UI. Unsupported/nonidentity surfaces explicitly return unavailable and offer Custom; there is no forced orientation or silent classification change.

Activity recreation/relaunch retains an immutable request/layout snapshot in saved state and a matching process-lifetime fallback, without View/Activity/Context/renderer references. A recovered client is **cancel/restore only**. A fresh native Idle discards a stale request; a different live identity is neither cancelled nor resized. The original layout is restored before the recreated unowned holder starts. An old-generation Complete race can only witness current layout/Entry; it cannot resize a currently rendering holder or manufacture current completion.

Checks actually completed:

- **77 ownership/output host checks, zero failures.** Stale ACKs/identities, observed profile dimensions and rollback are covered without changing the existing deadlines.
- **48 focused Java tests** in seven classes, zero failures/errors/skips. Includes holder/no-op ordering, original layout margins/RTL, Activity saved/no-bundle recovery, different-owner rejection and completed-generation races.
- Native Debug compilation for **arm64-v8a, armeabi-v7a, x86 and x86_64** passed. APK packaging passed. Final lint has **zero errors,80 warnings**, without suppression.
- Exact12 source hashes, build stages and APK identity are in `receipt.json`. APK is206135428B, SHA256 `6ce0d20e8104b2a394a0784b470b526238ad4b0524b668d5a70e2965c310f28e`. This is a Debug multi-ABI artifact, not a Shipping installer size.
- Package inspection:204 entries, four native libraries, no source masters/FLAC/ZIP/Blend, docs/reports or parked Bellwetherv3/v5 assets. See `package.json`. The previous Debug byte count has no sealed entry inventory, so its whole size difference is not attributed to this feature.

## Retained failures

The first full build compiled all native/Java code but failed one of48 tests: exact layout restoration serialized undefined RTL-relative margins as explicit values. Public clone-only margin probing and skipping undefined setters corrected it; no reflection or assertion removal was used. The focused nine-case rerun passed.

A later Java/package pass had48 passing tests but one new `SuspiciousIndentation` lint error. Explicit braces corrected the conditional cancellation catch without changing its predicate; final affected tests/lint/package pass. The compact [failure record](retained-failures.json) remains; original XML/logs are kept locally under ignored `reports/android-benchmark-fixed-output-20261024/`.

## Required actual acceptance

On an authorized phone: verify supported orientation/transform, actual SurfaceHolder/compositor sizing and letterboxing, owning completed640x360/1280x720 RT evidence, truthful profile receipt/course report, no-op/Back/Home/rotation/Activity-recreation/relaunch paths and exact personal settings/layout restoration. Unsupported cases must remain unavailable rather than approximate. Do not infer these from Robolectric, compiler success or Windows evidence.

Audio/haptic manual revalidation required: **YES**, for Entry/Showcase/Entry transitions and interruption listening. Existing forest music approval does not cover this transition. The feature remains provisional and unadmitted until its actual platform gates pass.
