# Horde Lantern RT 1.7.0 release validation

Runtime source: `d1c6d152d18c7002c41638d51dc9d9bcceea1bd4`.
Android versionCode 11. These are ordinary shipping-content builds; private revival, flask-candidate and Windows acceptance modes are disabled.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| Horde-Lantern-RT-Alpha-1.7.0-Windows-x64.zip | 267,719,165 | `e06a7a9d67607586447fbaaa46e60d5465de3c19fc99d22aefffd5d1f28dd277` |
| Horde-Lantern-RT-Alpha-1.7.0-Android.apk | 232,380,998 | `2767704fac89d6670099a4ff5ff396dc7206910a092148a3c8708193dca226be` |

## Build and package evidence

The complete official package gate passed at baseline627c6485:265/265 host tests, all Android Debug/Release ABIs, lint, archive, asset/licence, pinned derivative and16KiB guards. This is baseline evidence, not a claim that the one-shot packager ran on d1c6d152.

Final d1c6d152 Windows Release and all four Android Release ABIs/lint passed. Three affected host tests and30 affected Android menu/summary tests passed. The final Windows archive replaces only its executable and release notes; all225 other entries are byte-identical to the validated baseline. Final version/manifest, music, notices, runtime assets, pinned1.7 derivatives and held-item guards passed.

The Android APK uses the established signing certificate and passed signature/alignment checks. In-place installation retained the existing first-install identity. The owner reports a Google Drive backup of the signing folder; independent password/recovery confirmation remains owner-attested, not agent-inspected.

## Physical acceptance scope

Owner Windows acceptance includes Kit walking and conditional basin dialogue, death/Retry twice, abandoned-practice Keeper admission, return to main menu and benchmark availability/completion. Earlier Android owner acceptance at627c6485 covers earned checkpoint save/reload, touch refill and clearing the live-combat notice. These are specific observed checks, not universal gameplay or device certification.

On final d1c6d152 SM-S948B, Standard completed2laps/26waypoints/1838 measured RT frames after Graphics→Back. Actual preset output1280x720 and RT640x360 were verified. Completion and cancellation restored original1440x2971 output and360x743 RT at saved25% scale. The fixed widescreen presentation in portrait was observed and accepted by the owner for this build.

## Publication status

Final current-build Custom completed at saved 25% settings (360x743 RT / 1440x2971 output). One explicitly consented public submission received a matching published receipt; exactly one matching report row was independently read back from the live public API. No prior report was resubmitted. The detailed [SM-S948B compatibility entry](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#3-november-2026-final-published-170-standard-and-public-sharing-acceptance) retains the exact device/settings/cancellation scope.

## Published receipt — 3 November 2026

[GitHub v1.7.0](https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.7.0) became public at **09:57:19 UTC**, targeting runtime source `d1c6d152d18c7002c41638d51dc9d9bcceea1bd4`. GitHub reports both assets uploaded, with byte counts and SHA-256 digests matching the table above. `SHA256SUMS.txt` accompanies the downloads. Published assets and the version 1.7.0 / code 11 identity are frozen; this documentation closeout does not rebuild or replace them.

The [itch project](https://samfa12.itch.io/the-horde) has both processed 1.7.0 channels: Windows build **2286893** and Android build **2286895**. The main page and [1.7.0 devlog](https://samfa12.itch.io/the-horde/devlog/1749252) were published after the update went live. The public [benchmark board](https://samfa12.com/games/the-horde/benchmark-stats/) is linked from the release communications.

PR37's checked source was integrated into main at `a94a87167765645a75997f40f94533527fd86f75`; the tag's runtime source remains `d1c6d152`, while later provenance/documentation commits remain separate. Required merge checks and post-merge CI have their own exact commit/run receipts; publication does not certify future source changes. The owner's explicit publication authorization and these verified public endpoints establish release status, rather than hashes or a prior candidate checklist alone.
