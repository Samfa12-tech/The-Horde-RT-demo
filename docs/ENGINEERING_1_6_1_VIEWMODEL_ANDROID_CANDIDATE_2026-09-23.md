# Isolated Android viewmodel candidate — not accepted

This is development packaging and host evidence, **not** a phone, art or performance
pass. ADB enumerated no devices on September 23, so no install was attempted.
The normal production/block-arm route and both tracked runtime GLBs are unchanged.

## Candidate and ownership

The owner identified the source gauntlet as Right. The selected test export uses
`--gauntlet-source-hand Right --grip-roll-degrees 105 145 --blend-elbows`;
Left/torch is reflected geometry and Right/sword is unmirrored. The roll values
are experiments, not accepted calibration. See the [hand record](ENGINEERING_1_6_1_HAND_ORIENTATION_2026-09-23.md).
The additional fitted-sleeve experiment is not packaged.

`-PhordeViewmodelCandidateDir=<export directory>` enables only Debug's asset
overlay, separate app ID `com.samfa12.hordelanternrt.debug.viewmodel`, and label
`Horde RT Viewmodel Candidate`. It validates the paired world/viewmodel hashes,
source-world authority, role and Right-source receipt before staging. The world
counterpart uses the existing world slot; the viewmodel retains its own small
vertex buffer/BLAS/TLAS ownership and shares the gameplay-solved animation/IK/grips.
Live play, normal checkpoints and route replay select ModelledViewmodel only in
this opt-in app. Explicit development routes retain their own selection.

Normal Debug stays `.debug`; Release stays `com.samfa12.hordelanternrt`. Release
explicitly disables the native candidate flag and consumes no Debug overlay,
even when the candidate property is supplied. Native configuration rejects the
candidate flag outside checkpoint-enabled Debug. No app data is cleared.

## Exact local artifacts and checks

Built from the working changes based on `b352599`; later provenance/testing
commits must not be presented as a clean-source rebuild of these exact bytes.

| Evidence | Result |
| --- | --- |
| ARM64 candidate Debug build | Passed; actual Gradle artifact is under `app/build/intermediates/apk/debug/`, not the stale conventional output directory |
| ContextualControlsLayoutTest fresh rerun | 4 tests, zero failures/errors; includes all eight viewmodel checkpoint name/ID mappings |
| ARM64 unsigned Release build with candidate property | Passed, including lintVital; candidate flag OFF, zero viewmodel ZIP entries, original world hash retained |
| APK resolver | 10/10 fixtures: ordinary/redirected outputs and malformed/missing/traversal rejection |
| Candidate negative gates | Gradle rejects independently wrong source handedness and runtime hash; real Android/Clang CMake configuration rejects candidate ON in Release after compiler checks |
| Six gauntlet mirror/UV tests | Passed afresh; source Right mirrors only for Left, winding/UV association preserved |
| Physical SM-S948B | Not run: ADB lists no devices; no installed-byte, presentation, capture, lifecycle or live-motion result |

Debug APK SHA-256:
`6047b285d6507ce84ccc90b48faa52b07cc917446167dc271f5bc6ebd244b2f9`.
Immutable local copy:
`C:/Dev/tmp/horde-android-viewmodel-candidate-20260923-a/HordeLanternRT-1.6.1-viewmodel-debug-arm64.apk`.
`aapt` confirms the separate candidate app ID. It contains one ARM64 native library,
the viewmodel manifest/GLB/receipt, paired world
`4050e15b5084bfa83ad9db09f8bb8ececd872be89a27d0b08447c12f9f7a501d`
and viewmodel
`194aab2874993c68b0ab5950ed82085564e8567b9a5f8126f27ceb11283f5e06`.

Unsigned Release isolation APK SHA-256:
`7a852b26fd7dcbae7909b5fc066aeec9cf7ede93812db3fb21c26d5bb540327f`.
It retains original world
`e8737f10e7669b284e04109d9c3acdf537df284093a21511656bf191a70450fd`.
This is ARM64 build/package isolation, not a fresh four-ABI or final-candidate gate.

Local logs: `reports/phase3-android-viewmodel-build.log`,
`reports/phase3-android-release-isolation-build.log`,
`reports/phase3-android-viewmodel-unit-fresh.log`, and
`reports/viewmodel-candidate-guard-validation-2026-09-23.md`.
Logs/APKs remain local; GitHub source is not a backup of compiled artifacts.

## Rebuild and pending phone matrix

Generate a fresh export directory with `tools/process-player-viewmodel-runtime.py`
and the flags above; it first verifies the admitted world reference. From `android/`:

```powershell
.\gradlew.bat :app:assembleDebug '-PhordeViewmodelCandidateDir=C:/Dev/tmp/horde-calibrated-right-arms-20260923-a' '-Pandroid.injected.build.abi=arm64-v8a' --console=plain
```

Do not assume `outputs/apk/debug/app-debug.apk` is current. Use
`tools/resolve-android-apk.ps1`, which follows Gradle's listing, or preserve an
immutable exact APK and supply `-SkipBuild -ApkPath`. The validation runner checks
the package identity **before install** and the installed APK hash afterwards.

Once the already-authorised exact phone is visible, use the existing runner with
the immutable artifact, candidate directory, explicit serial/model, and eight
captures: `player-viewmodel-grips`, `player-viewmodel-forward`,
`player-viewmodel-downward-cut`, `player-viewmodel-upward-slice`,
`player-viewmodel-look-up`, `player-viewmodel-look-down`,
`player-viewmodel-lantern-high`, `player-viewmodel-lantern-low`.
Keep ordinary two-enemy/replay coverage and Home/resume. Capture gates require
ModelledViewmodel, selected 60 Hz skinning, actual skin updates, 21-slot capacity
and socket error at most 0.015 m. Android's report does not expose individual
masks; native host route tests remain separate mask evidence.

Then inspect live walking/look, downward/upward cuts, torch wall retraction,
reward pickup and swinging lantern. Judge thumb side, palm/knuckle orientation,
cuff continuity, sleeve edges/folds, prop intersections and scale on both hands.
Frozen poses cannot replace that motion check or the owner's subjective approval.
No Shipping timing gain, backend parity, S24/S25 support or phase completion is
claimed. Audio/haptic manual revalidation required: **NO** for this slice; event,
listener, playback and haptic semantics are unchanged.
