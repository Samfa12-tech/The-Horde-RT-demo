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

Final current-build Custom completed at saved25% settings (360x743 RT /1440x2971 output). One explicitly consented public submission received a matching published receipt; exactly one matching report row was independently read back from the live public API. No prior report was resubmitted. Six required hosted checks remain a merge-time gate. Do not treat this validation record or its hashes alone as publication approval evidence. Runtime source and later documentation/provenance commits are deliberately distinguished.
