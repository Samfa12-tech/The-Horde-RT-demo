# Rescue and forest music continuation - 20 October 2026

The shared resolver now distinguishes the playable chapter from the legacy
showcase ending. The existing WP2 worktree, branch and draft PR 27 continue.
This checkpoint reuses the admitted owner A-H pack and accepted PCM backend;
there is no new music asset, score, synthesis, audio engine or save field.

## Result

| Chapter state | Selection |
| --- | --- |
| Keeper defeated, still below | F, including lantern claim/reveal and stale legacy ending phases. |
| Observed first completed rope ascent with the lantern | G once from its existing beginning, then H after its existing six-second body. Overshoot advances H by the remainder. |
| Exterior/forest after the first arrival | H; its existing twelve-second body loops. |
| Return below, including return during G | F; the return cancels unfinished G. A repeat ascent selects H directly. |
| Fresh attachment or lower/upper/Forest checkpoint Load | Select F/H directly from committed chapter state. No invented live-arrival reveal. |

The existing audio content clock owns G's duration. Pause, external menu and
focus suspension freeze the current cursor; ordinary resume does not consume
suspended wall time or seek from legacy `finale.phaseTime`. Chapter mode changes,
retry and explicit reset discard transient reveal/latch state while revisions
remain monotonic. The non-chapter showcase keeps its existing G-to-H policy.

Windows now publishes a new semantic-event lifetime after successful campaign
Load/New Game replaces the queue. Previously its hardcoded lifetime could leave
the inbox's old sequence high-water above the replacement queue's new events.
Ordinary menu, pause and profile publication keep the existing lifetime. Android
already marks actual queue replacement. Volume, mute, output gains, focus rules,
Core ownership, PCM transitions, haptics and campaign record format are unchanged.
An explicit Load starts a fresh music session; music cursors are not saved.

## Verification and exact identity

- **65 new host checks pass**, including the actual imported terrain and shared
  rope motor, lower/upper/completed-Forest logical restores, natural shared finale
  phase progression below, exact G boundary/H loop, early return/repeat ascent,
  pause/menu/resume, death/retry/mode/reset and monotonic publication identity.
- A synthetic PCM check uses the real session and Core path. One buffer contains
  the last G body sample followed by the first H sample plus G's tail, with no
  gap. It also checks frozen PCM/menu identity and explicit queue replacement.
  Synthetic samples do not validate the admitted music's perceived transition.
- **13/13 affected Release CTest registrations pass** in 36.46 seconds. This
  includes legacy resolver, PCM stream/WAV/bank/session, Windows focus/publication,
  campaign session/codec/checkpoint and rescue/terrain suites. It is a selected
  lane, not the complete repository gate.
- **19 selected Java tests pass**: MusicAudioFocus (9), MusicOutputBuffer (5) and
  MusicOutputGain (5), with zero skips/failures/errors in retained XML. These are
  mocked/Robolectric output-boundary contracts, not platform listening.
- Windows Debug/Release application builds and the four-ABI Android Debug build
  are identified in [the receipt](receipt.json) and their retained logs. Android
  uses Shipping instrumentation and Mobile dielectric quality; no device is used.

The [source freeze](source-freeze.json) records all seven changed source/build/
test files before the final checks and application builds, including normalized
Git blobs. The receipt verifies they still match and pins binary/APK/test hashes,
the four packaged ABIs and every admitted music WAV inside the APK. Documentation
and evidence are added afterward. No large generated binary is committed here.

The [asset audit](music-assets.json) verifies all sixteen runtime WAVs against
their admitted manifest and Git LFS object identities: stereo 48 kHz PCM16,
twelve-second bodies except C (three) and G (six), and three-second tails.
Runtime WAV bytes remain **20,160,704**; decoded PCM remains **20,160,000**.
Editable JSON/PCS1 sources and tracked Core bytes also match. No asset is fetched,
changed or played.

The first asset helper failed by comparing a hydrated WAV directly with its
132-byte Git LFS pointer. Its exact failed helper and trace are retained under
`initial-asset-audit.py` and `asset-audit-first.log`. The corrected helper compares
the actual SHA-256/size with the LFS oid/size and manifest. This is an audit-helper
failure, not an audio corruption finding. No assertion, asset or pointer was
weakened to obtain the subsequent pass. The final source review found no
actionable defect; no additional runtime or listening claim follows from it.

## Reproduction

Use the existing Windows Release preset and build the targets named in
`host-tests.log`, then select exactly those thirteen CTest registrations with
`--output-on-failure --no-tests=error`. Application target:
`cmake --build --preset windows-x64-release --target horde_rt_diagnostic_window --parallel 1 -- /nr:false`,
then the same command with `windows-x64-debug`.

From `android/`, with the configured shared JDK/Gradle/SDK:

```powershell
gradle.bat --offline --max-workers=2 :app:testDebugUnitTest --tests com.samfa12.hordelanternrt.MusicAudioFocusTest --tests com.samfa12.hordelanternrt.MusicOutputBufferTest --tests com.samfa12.hordelanternrt.MusicOutputGainTest -PhordeRtInstrumentationOverride=Shipping -PhordeRtDielectricQualityOverride=Mobile --console=plain
gradle.bat --offline --max-workers=2 :app:assembleDebug -PhordeRtInstrumentationOverride=Shipping -PhordeRtDielectricQualityOverride=Mobile --console=plain
```

`python audit-music-assets.py` can run from this evidence folder; its output is
written beneath ignored `reports/`. Native save fixtures ran under normal host
permissions in isolated temporary locations, without owner records.

## Remaining acceptance and next work

**Audio/haptic manual revalidation required: YES.** Cue/state selection, timing
and Windows event transport changed. Haptic patterns/routing did not change.
Actual listening remains open: F-to-G-to-H suitability and mood, boundaries/tails,
speech/combat intelligibility, volume/mute, ordinary menu/focus resume and explicit
Load on each platform. Reusing H as the exterior/forest bed is implemented, not
an owner listening approval. No platform audio device was opened in this slice.

The real rope motor host fixture begins from a logical lower checkpoint; neither
it nor shared finale progression is a naturally won Keeper fight or physically
earned save. Natural full-route/return, earned saves/death Load, first-time
teaching/contact, touch/controller comfort and sustained warm Keeper/exterior
performance remain physical gates. No RT presentation or new performance gain
is claimed by these audio checks/builds. Sam's mobile **Mist Off** choice remains
in force; the two-light Keeper workload is not accepted from cleared-tomb timing.

The next bounded independent source slice is an audit of existing licensed local
tomb/rope ambience and one real-location/event path if suitable admitted cues
exist. Forest species/waterfall paths already exist. Healing remains deliberately
undecided: the [morning decision record](../../design/HORDE_1_7_RECOVERY_DECISIONS_FOR_MORNING_2026_10_20.md)
lists the exact method/capacity/refill/use/death/migration choices and current
health-reset behavior. No policy was guessed. Debris kicking/option and whole-
opening art remain distinct gaps; avoid heavy outside expansion.

Canonical owner licence/memory bytes and unrelated assets are preserved. No
phone query/install/launch/transfer, denied contact retry, public push, release,
purchase, new checkout, reset/stash or Reef operation occurs in this slice.
