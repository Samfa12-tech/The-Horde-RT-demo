# Finite forest foliage cue - 20 October 2026

One restrained leaf-rustle cue now uses the existing single finite chapter
incidental voice. This continues the WP2 worktree and draft PR 27; it adds no
continuous wind bed, renderer wind, new audio layer or saved state. The prior
[pine LOD2 comparison](../2026-10-20-pine-lod2/README.md) stays rejected and current
LOD1 stays in production.

Working directory:
`C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.7-wp2-vertical-proof`.
Branch: `codex/horde-1.7-wp2-vertical-proof`; build base: `6e363730`.

## Behavior and ownership

The shared 60 Hz forest scheduler uses an empty slot: first rustle after 44 quiet
seconds, then every 40 seconds. Existing owl/cricket/pond slots remain. Each cut
lasts 0.90173 seconds, so scheduled rustle occupancy is below 2.3%. Eligibility
requires the existing live, grounded, ready same-night forest route, equipment
out, no speech/pause and proximity to the accepted route pine at placement 7.
Its fixed source is approximately `(-22.29810, 4.55, 17.69911)`, in the existing
lower needle crown; geometry, root contact and collisions are unchanged.

The strict planar range is 12 m, with a linear outer 4 m eligibility gain and
emitter gain 0.4. Existing planar spatialization uses reference distance 4 m and
maximum distance 12 m. Source/listener height metadata is retained; elevation
mixing is not added. Existing walkable-rectangle obstruction also applies outside
and contributes its existing 0.38 multiplier. Consequently these parameters are
structural candidates, not a verified audible level.

Missed slots are dropped without catch-up. Ordinary ready forest-zone transitions
do not restart the scheduler. Leaving the source range stops an active rustle
without resetting the other nature cadence; this stop consumes one quiet fixed
tick. Broader eligibility loss/session replacement resets transient cadence, so
reentry waits the initial quiet period. The scheduler retains the most recent
finite cue identity after its PCM has ended; a later stop remains harmless.

Semantic event 35 is appended without renumbering IDs 0-34. Both native consumers
use one emitter helper; all prior event types retain the 1/14 m emitter contract.
Latest-signal coalescing and current-publication eligibility prevent stale starts.
Android checks on the render owner, preserving immutable publication/JNI ownership;
its existing focus callback now stops chapter ambience as well as menu ambience.
Focus eligibility remains independent of the saved Music volume. Windows keeps
its current foreground gate. Both platforms stop before playing through the
existing finite voice and apply SFX gain once; Android uses loop-count zero and
Windows uses finite Play. Missing/unloaded clips are dropped. Dialogue ducking,
saved gains, haptic patterns and campaign save fields are unchanged.

## Licensed material and reproducibility

The complete private FilmCow `leaves rustle 1.wav` supplies 43,283 mono 48 kHz
PCM24 source frames. Runtime output is **86,610 WAV bytes / 86,566 PCM bytes**,
mono 48 kHz PCM16, with 480-frame fade-in and 3,840-frame fade-out. There is no
normalization, filter, synthesis, pitch or time-stretch. Measured output peak is
-14.016 dBFS, RMS -41.034 dBFS; both endpoints are zero and no sample clips.

The [manifest](../../../assets/audio/forest-foliage/asset.manifest.json) pins source,
runtime, recipe and existing licence PDF identities. The custom FilmCow project-use
licence is recorded in [ASSET_LICENSES](../../../ASSET_LICENSES.md); it is not CC0.
Originals remain private and untouched. Five leaf-labelled originals were inspected
for PCM statistics; the other four are alternatives, not audible-quality rejections.

`python tools/prepare-forest-foliage.py --verify` reproduces both runtime products
byte-for-byte without writing. An independent exact Fraction calculation matches
all 43,283 quantized/faded samples. Sizes, hashes and modification times of both
outputs and all five originals are unchanged by verification. See
[reproducibility.json](reproducibility.json) and [source-statistics.json](source-statistics.json).
No local listening tool was available, no output device was opened, and no
playback/listening was performed. Filename and PCM inspection do not establish
character, audibility or mix suitability.

## Verification and retained failures

- The new native foliage fixture passes **148 checks**, covering finite/range/
  lifecycle/speech gates, source metadata, gain composition, all prior event
  emitters and cadence under 15/24/30/60/120/144 Hz render groupings.
- The existing ordinary CPU journey reaches the real forest through guards,
  Keeper, reward, rope and presented-lantern proof. Seven added checks observe
  two actual shared-simulation rustles at the accepted route point, 40 seconds
  apart. That suite now passes 101 checks. Three added source integration guards
  bring local ambience to 110 checks. These are CPU fixtures, not natural player
  acceptance or RT playback.
- **14 unique selected Release CTest registrations pass across retained runs**,
  including campaign codec/storage/checkpoints, shared gameplay, forest/local
  ambience, music, dialogue ducking, Windows focus, spatial transport, motion
  evidence and four Python asset-policy tests. This is not one 14/14 run or the
  full repository gate. Original outputs and check counts remain in the logs.
- **31 selected Java tests pass**, zero failures/errors/skips: chapter feedback
  5, menu ambience 7, music focus 9, output buffer 5 and output gain 5. XML reports
  are retained under `java/`. These are host/mocked tests, not device playback.
- Windows Debug/Release and Android Debug Shipping/Mobile builds pass. Android
  contains arm64-v8a, armeabi-v7a, x86 and x86_64; no installation occurs. Exact
  executable/APK hashes and sizes are in [artifact-receipt.json](artifact-receipt.json).
- The existing Windows night-audio helper stages all ten admitted runtime/metadata
  files byte-exact twice. APK entries match those bytes and exclude private source
  filenames. No release archive is produced. Prior runtime PCM, saved INIs and the
  canonical owner's two dirty files retain their checked identities.

The first Release command requested a nonexistent target after its initial targets
built (`build-release-initial.log`). The corrected target roster and final builds
pass. Initial CTest output retains two failures: an outdated source-text spatial
assertion after the emitter refactor, and Windows access denied (native 5/status 15)
in the storage fixture's exclusive disposable temp directory. The assertion now
checks both helper consumers and actual height/default-range metadata; its rebuilt
test passes. The unchanged storage fixture passes with normal Windows temp access;
no storage policy/assertion was weakened or owner record touched. The first Gradle
attempt failed to load native-platform.dll before starting. The same offline task
with normal cache access passes. No automatic-approval rejection occurred.

[source-freeze.json](source-freeze.json) records final raw working-file identities
for 23 source/asset files, after the source-assertion correction and before final
builds, plus the prior audio/settings/owner preservation checkpoint. Existing
audio README edits predate this checkpoint; the claim is preserved runtime PCM,
not an unchanged README. Git may normalize source line endings; these hashes name
the actual built working bytes. Documentation added after builds is not in that
source freeze. Evidence files retain raw bytes; `evidence-sha256.json` names their
identities without including itself. No EXE, APK, original archive or duplicate
runtime WAV is committed in this evidence directory.

## Remaining acceptance and next accepted item

**Audio/haptic manual revalidation required: YES.** Listen on Windows and Android
for leaf character/audibility beside water, steps and Kit; source pan/range, saved
SFX and Music zero/mute, pause/menu/focus/surface loss, route exits and load/restore
cleanup. Haptic patterns are unchanged. Structural tests do not prove actual
output cleanup, exact-candidate feedback feel, natural saves or warm performance.
Mobile Mist Off remains owner-selected. No phone, public push or release occurred.

Next unblocked source item: inspect the accepted **opening tomb burial-story
composition and grounding in actual native RT**, then correct at most one
demonstrated placement/path-readability defect using existing owned assets and
budgets. Keep route, collision, landmark visibility, lights and saves intact; if
no defect is demonstrated, record that result rather than inventing dressing.
The six [morning recovery choices](../../design/HORDE_1_7_RECOVERY_DECISIONS_FOR_MORNING_2026_10_20.md)
remain pending. Natural route/save/death, listening and sustained warm acceptance
remain the higher-priority owner testing gates; no new system is selected.
