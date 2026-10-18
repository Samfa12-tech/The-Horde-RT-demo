# Bounded physical crouch and lifecycle acceptance

The finite ordinary-opening crouch pass succeeded on Samsung **SM-S948B**,
Android **17 / API 37**, user 0, build `CP2A.260605.016.S948BXXS4BZIG`.
This is source commit `1906a733b8b401672643c7f22d0327a500bdd211`, built as the
frozen crouch/root working diff over `1bf8921a565dd92f40ca2c8b4c6edf651ff0e2c9`
before that local commit. The [implementation receipt](../2026-10-18-crouch-playable/README.md)
records the matching source/build hashes and separate host checks.

## Exact artifact and preservation

Separate Debug package `com.samfa12.hordelanternrt.debug`, `1.6.2-debug`, code 10,
183,853,322 bytes, APK SHA-256
`b1f266f69adf6b92eda2c18fe7c02aa967cbb626d853ef4dd910ab1394f82211`.
Public V2 certificate SHA-256 remains
`2bc406088b23ac80f2a00a40e2d5d8daa012844dc62f31ad6145d129f5566736`.
The on-device previous APK matched the signer-verified
`6d865599c74828bc15165693e7ea23e31454e2909c7ddd0b66123cf0534809d6`
[Back-test artifact](../2026-10-18-android-back-device/README.md). Package/version
and public signer were checked before `adb install -r`; installation succeeded
and the installed hash matched the candidate. No redundant APK copy/pull occurred.

The launcher was foreground with no Horde activity/task. The remaining PID 25338
was the prior test's frozen, empty cached process, not an owner-running session.
No owner process was killed. No desktop input, Reef interaction, release-package
action, uninstall, data clearing, security-setting change or save operation occurred.

Horde settings remain byte-identical before, immediately after installation and
after terminal Quit, SHA-256
`e3966b09cd9c354cb92833496b7f812139178dd7b2a906a4f809dbf9538c3dee`.
The `files/`/`shared_prefs/` path inventories are identical at 274 paths, with no
save/campaign/checkpoint-named path. This is not whole-app-data byte equality;
ordinary cache, framework and report contents may change.

## Finite ordinary-play sequence

The Activity launched normally, with no debug automation, checkpoint, quality or
save extras. PLAY was tapped at its observed UI bounds. No movement/look input,
attack, dodge, run, restart, settings or benchmark action was sent.

| Observation | Result |
| --- | --- |
| First sample after PLAY | Still `Opening the ruin…`; retained as a pending-readiness sample, not standing acceptance |
| One bounded readiness check | Ordinary live HUD shows enabled `Crouch`, plus Menu/Dodge/Run/Parry/Swing and 3/3 hearts |
| Tap Crouch | Native contextual state changes the button to `Stand` |
| Tap Stand | Native contextual state returns the button to `Crouch` |
| Crouch, then Back pause, then Back resume | Observed pause menu; resumed native HUD is `Crouch`, clearing the stance request |
| Crouch, then Android Home and explicit app resume | Initial resumed surface lacks the stance control while rebuilding; retained. One bounded readiness check returns the native standing `Crouch` HUD |
| Terminal cleanup | Back opens pause menu; one scroll within its observed ScrollView reveals `QUIT DEMO`; explicit Quit removes the test activity/task |

Crouch/Stand labels are derived from accepted native simulation contextual
bits, not an optimistic Java toggle. The app-PID/device-time-scoped log has no
fatal exception or signal. It records successful native RT surface presentation
for the opening and resumed generation 3, with 576x1188 dispatch into 1440x2971
swapchain at the unchanged saved 40% scale. These are presentation/lifecycle
facts, not sustained performance measurements or process-death recovery.

## Opened images and limits

The actual app-only screenshots were opened and inspected. Their exact paths,
sizes and SHA-256 hashes are recorded in [validation.json](validation.json):

- `reports/android-crouch-device-2026-10-18/03-live-standing.png`
- `reports/android-crouch-device-2026-10-18/04-crouched.png`
- `reports/android-crouch-device-2026-10-18/05-restood.png`
- `reports/android-crouch-device-2026-10-18/11-home-ready.png`

The crouched view reveals more floor and shifts the doorway/encounter silhouettes
upward; standing restores the earlier view. The held torch and both hands remain
visible, with no obvious detached/floating prop response in these specific views.
This is qualitative inspection, not pixel parity or owner art/comfort acceptance.
The ordinary route emitted no input-presentation JSON samples, and exposes no
live numeric eye height. The exact 0.6 m drop is therefore **not physically measured**
here. Torso/legs are not sufficiently visible for full-body mesh acceptance.

Physical root walking, blocked standing under a low ceiling, full-body mesh,
numeric eye height, physical keyboard/controller/gesture variants, broader font/
orientation behavior, sustained warm performance, audio/haptics and comfort
remain **UNACCEPTED**. No frozen forest capture or new performance trial was added.
Gameplay Save/Load, process-death save recovery and platform/power-loss durability
were not exercised. The prior checkpoint startup evidence belongs to its own APK.

Audio/haptic manual revalidation required: **YES for the posture implementation's
event-time listener/held-source changes**, remaining unaccepted by this automated
visual/HUD pass. No listening/comfort approval is inferred or requested here.

All raw UI XML, screenshots and logs remain ignored. This device pass changes no application source; its evidence is recorded separately
after the implementation commit. The initial loading and resumed-surface
pending samples are retained; no failing assertion, quality or deadline was relaxed.
