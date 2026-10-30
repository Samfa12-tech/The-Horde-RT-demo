# Shared dialogue bed balance - 20 October 2026

Music and waterfall now consume one smooth, transient dialogue multiplier owned
by the shared fixed-step simulation. This continues the existing WP2 worktree,
branch and draft PR 27. No audio asset, output backend, PCM/Core implementation,
stored volume, campaign resource field or graphics setting changes.

## Behavior

The envelope attacks over **0.20 seconds to 0.65 gain**, holds the current gain
through **0.35 seconds** of quiet, then releases over **0.90 seconds** with cubic
smoothstep. These are an implemented starting mix, not listening acceptance.
Consecutive line generations keep the ongoing attack/floor. An interrupted short
attack holds its actual gain; it does not keep ducking silence. New speech during
release starts at the current gain without a unity-volume pulse.

Only active unscaled dialogue time advances the envelope. Ordinary pause/menu,
focus and surface reconstruction preserve it. Explicit route/retry/checkpoint or
mode replacement clears it; campaign restore starts fresh and derives any new
attack from the restored logical active line. It is never a saved user setting.
Death/non-chapter snapshots return unity. Invalid snapshot gain also returns unity.

Music composes this gain with the existing Keeper envelope at the existing
output stage; cue identity, cursor, revisions, PCM/tails and restart ownership
remain intact. Windows waterfall applies it once with the existing SFX control;
Android publishes one coherent attenuated stereo tuple and retains its existing
Java SFX multiplier. Finite chapter incidentals already stop for speech. Voice,
combat SFX, menu-room ambience and haptic routing/patterns are unchanged.

The first new integration run exposed an inherited **Grate replay bug**. During
active speech the director repopulated its pending local request. Skip/completion
consumed the call but that request immediately restarted it, keeping the bed
ducked. The director now discards a consumed local request before start logic.
Unheard out-of-range re-entry and the separate ordered rescue/forest continuation
queue retain their existing behavior.

## Verification and exact identity

- **160 new host checks** cover smooth/continuous bounds, interrupted attack,
  15/30/60/120 Hz render grouping, real shared dialogue chains, skip/cancel,
  stale generations, long pause/resume, resets/mode/retry, ordinary Grate travel,
  actual-terrain logical campaign restores, synthetic PCM/cursor identity and
  both platform output wiring. Direct Grate Skip/Complete/fallback regressions
  seed several active ticks so the stale request actually exists.
- **18/18 selected Release CTest registrations pass**, with exact selection and
  duration in [host-tests.log](host-tests.log) and [receipt.json](receipt.json).
  This includes shared simulation, dialogue, music, local/species ambience,
  save/session/codec/checkpoint, rescue/terrain and existing gain/focus contracts.
- **41 selected Java tests pass**, zero skips/errors/failures: AmbienceOutputBuffer
  (7), AmbienceOutputQueue (8), MenuAmbiencePlayback (7), MusicAudioFocus (9),
  MusicOutputBuffer (5), MusicOutputGain (5). Retained XML identifies each case.
  Java production bytes were not changed; these mocked output contracts do not
  prove physical playback.
- Final Windows Debug/Release and Android Debug application builds pass. Android
  contains all four expected native ELF ABIs, with Shipping instrumentation and
  Mobile dielectric quality verified from each cache. Exact APK/EXE/library and
  representative test-binary identities are in the receipt.
- All sixteen existing music WAVs and both local ambience WAVs match their
  admitted manifest hashes inside the APK. Tracked audio, Core, Java and campaign
  format code remain unchanged. Canonical owner licence/memory hashes match.

The [final source freeze](source-freeze.json) records twelve source/build/test
files before the corrected native checks and final application builds. Raw SHA256
and normalized Git blobs are verified again before staging. Documentation and
evidence are added afterward. No generated application binary is committed.

The read-only final reviewer verified that freeze and the consumed-call guard,
original failing assertions and new direct regressions. The earlier source review
missed the inherited pending-request interaction; integration tests found it.
Review adds no playback, hardware or performance claim.

## Preserved failures

The [initial run](initial-host-tests.log) and [full initial output](initial-LastTest.log)
remain unchanged. It timed out in the restricted campaign disk-publication test,
and **146 initial ducking checks had two failures**: real Skip did not enter
quiet hold, then did not reach unity because Grate had restarted. The production
request bug was fixed; those original assertions remain unchanged. Added direct
regressions increase the final count to 160. No assertion was weakened.

The same campaign binary passed a [normal-permission rerun](campaign-rerun.log)
in 3.23 seconds; the final full selected lane also ran with normal local
permissions. No owner save records were used. The initial source hashes are
retained in [initial-source-freeze.json](initial-source-freeze.json).

The [documentation preflight record](doc-preflight.log) retains a combined patch
context rejection before writes and an exact-copy assertion failure: the copied
healing table used straight quotes where its original used curly quotes. The
copied table now matches exactly, and the original policy/current-health/save
audit is unchanged. Neither documentation failure was a game regression.

The first staging helper then failed a hard-coded log-count assertion after its
roster/source/byte comparisons passed. Its expected count and printed label were
wrong. Checking the explicit roster confirms all fifteen retained logs, byte-exact;
the correction and initial failure are also in the preflight record.

## Reproduction and acceptance boundary

From the existing worktree, build the seventeen target executables named in
`host-build.log`, then select the eighteen registrations named in `host-tests.log`
(the terrain journey registration reuses an executable). Use Release,
`--output-on-failure --no-tests=error` and normal permissions for isolated temporary
save fixtures. Build `horde_rt_diagnostic_window` with the existing Windows
Release and Debug presets. From `android/`, with the configured shared toolchain:

```powershell
gradle.bat --offline --max-workers=2 :app:testDebugUnitTest --tests com.samfa12.hordelanternrt.AmbienceOutputBufferTest --tests com.samfa12.hordelanternrt.AmbienceOutputQueueTest --tests com.samfa12.hordelanternrt.MenuAmbiencePlaybackTest --tests com.samfa12.hordelanternrt.MusicAudioFocusTest --tests com.samfa12.hordelanternrt.MusicOutputBufferTest --tests com.samfa12.hordelanternrt.MusicOutputGainTest -PhordeRtInstrumentationOverride=Shipping -PhordeRtDielectricQualityOverride=Mobile --console=plain
gradle.bat --offline --max-workers=2 :app:assembleDebug -PhordeRtInstrumentationOverride=Shipping -PhordeRtDielectricQualityOverride=Mobile --console=plain
```

**Audio/haptic manual revalidation required: YES.** Music/waterfall gain and
consumed-speech replay behavior changed; haptics did not. Actual listening must
cover depth/timing, consecutive speech, skip/cancel, pause/focus/menu return,
checkpoint Load, mute/zero, tails and speech/combat intelligibility on both
platforms. No output device was opened or sound played in this slice.

Logical terrain-backed restores and CPU gameplay fixtures do not establish a
naturally won Keeper fight, physically earned save, full route/return, device
lifecycle or controls comfort. Mobile **Mist Off** remains owner-selected.
Sustained warm Keeper with both lights and exterior performance remain open;
the earlier cleared-tomb ~18% gain is not evidence for that combined workload.
No RT presentation or new performance gain is claimed here.

The [complete remaining-scope inventory](../../design/HORDE_1_7_REMAINING_SCOPE_2026_10_20.md)
separates substantial unfinished recovery/outdoor/content work, exact-candidate
acceptance and conditional details. It includes all six exact healing choices,
without choosing policy. Next resolve those choices and the final 1.7 disposition
of unfinished outdoor effects; then implement one bounded recovery slice and
validate natural route/save/death and warm performance when authorized. This
checkpoint adds no authority for heavy outside content or another polish loop.

No phone query/install/launch/transfer, denied contact retry, public push, merge,
release, purchase, new checkout, reset/stash, owner cleanup or Reef operation.
