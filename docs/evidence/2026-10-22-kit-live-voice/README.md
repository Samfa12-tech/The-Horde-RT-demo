# Kit live speech anchoring - 22 October 2026

Base `c2bbc71427972582f1b2ac973d23f17eeb763835`, existing
`codex/horde-1.7-wp2-vertical-proof` worktree and draft PR 27.
This corrects source/listener placement, not a new gain or mix policy.

## Demonstrated defect and correction

Reunion speech used the old fixed source `(-32,3.6,-10)` while the grounded Kit
root is approximately `(-27.7,1.39464,-5.8)`. Forest speech used route landmarks
instead of the moving actor. Source/listener/yaw froze at line start. Android
already recalculates its existing SoundPool stream gains; Windows returned for
the same line/generation before recalculating its output matrix.

The shared owner now binds reunion/forest speech to the grounded companion root
plus the existing 1.4 m speech-height convention and current listener/yaw. It
refreshes after companion movement and before snapshot publication. A restored
line binds the restored grounded Kit even in its first paused snapshot. Missing
actor data retains the last source; nonfinite tuples do not partially publish.
Active pause/death guards and the fixed Keeper, grate and unseen shaft calls
retain their prior contracts. Ordinary event-time SFX tuples are unchanged.

Windows now updates only the matching `chapter_dialogue` line/generation matrix
in place, including before resume. It does not stop, submit or restart PCM,
change voice volume, or fabricate a completion when no matching voice exists.
The Android stream/JNI interfaces require no change. Story order, line duration,
callback generations, continuations, save schema and ducking remain unchanged.
Base gain .36, 1 m reference/14 m range and the existing planar spatial law are
preserved. Owner Speech100/SFX20/Music55 preferences are not edited.

## Measured host spatial result

The isolated production-simulation fixture imports actual chapter terrain,
restores an unfinished reunion line, verifies current anchors across 15 ordinary
movement/turn ticks, pause and authored-boundary capture, then independently
restores the landing and walks to the reunion with ordinary collision-resolved
input. Its owner explicitly publishes the next zone's already-loaded terrain
readiness. No new story-completion flags are granted during the walk.

| Location | Old source distance | Correct actor distance | Old L/R gains | Correct L/R gains |
| --- | ---: | ---: | --- | --- |
| Restored rope landing `(-33.7,-12.8)` | 3.27567 m | 9.21955 m | .109187 / .012507 | .0383516 / .00733917 |
| Live reminder near `(-28.7023,-5.80325)` | 5.33736 m | 1.00233 m | .0459493 / .0493765 | .355250 / .0528804 |

These are calculated spatial coefficients from actual simulation snapshots,
not output-device loudness measurements. At the nearby reminder the attenuation
amplitude ratio is approximately 5.33 (14.5 dB), with corrected directional pan.
At the landing Kit is farther away than the obsolete point, so gain decreases.
This is not a blanket speech boost or a perceived-mix acceptance claim.

## Checks and limitations

- Four focused Release CTest registrations pass: chapter dialogue, campaign
  checkpoint, spatial audio and dialogue ducking. The checkpoint fixture reports
  1,058 checks/zero failures; ducking reports 160/zero. [Exact output](host-tests.txt).
- Windows Release `horde_rt_diagnostic_window` compiles/links. Android arm64
  native compilation passes, build-only. [Android output](android-native-build.txt).
  [Receipt](receipt.json) pins source and exact artifacts; no binary is committed.
- Windows identity/matrix checks are source guards, not endpoint playback tests.
  Existing capture modes pause speech; hidden campaign validation suppresses it;
  the Shipping hold explicitly mutes it. The separate music endpoint fixture
  cannot certify Kit's mono speech. No Kit endpoint playback or listening was
  performed here. Phone access was not attempted after owner disconnection.
- This is a seeded post-Keeper host fixture, not natural victory/claim/ascent,
  RT presentation, physical lifecycle, sustained performance or phone listening.
  It does not replace owner acceptance at the reported mix or prove intelligibility.

Audio/haptic manual revalidation required: **YES**, because continuous Kit
spatialisation and Windows output matrices change. Speech/listening and physical
pause/resume remain pending; haptic routing is unchanged.

Original fixture/build failures are retained in [failures.txt](failures.txt),
[initial output](initial-tests.txt), [first route output](route-fixture-failure.txt)
and [readiness diagnosis](readiness-fixture-failure.txt). No existing acceptance
assertion or production readiness/collision/save guard was removed or relaxed.

## Remaining work

Next desktop slice: diagnose the inherited shader/Android source-contract
failures against their actual implementations and address PR conflicts safely.
Keep the existing Debug route timeout open. The skin candidate remains a
6.73-8.33% desktop vertex-stage improvement with a narrowly missed target and no
Android/full-frame claim. Saved phone 12.06 ms skin-stage evidence and its
unresolved closing-control slowdown remain unchanged. Natural journey/recovery,
final touch UI, listening, warmed-phone performance and unspecified flask policy
remain separate open gates. No merge, release, phone work or new heavy audio cue.
