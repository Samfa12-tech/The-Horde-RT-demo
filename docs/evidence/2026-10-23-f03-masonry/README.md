# Grounded F03 treasury-road cue

The [inspected baseline](../2026-10-23-f03-inspection/README.md) showed a clear
forest trail but no marked masonry corresponding to Kit's road clue. This slice
adds eight short kerbs and one marked waystone beside the existing F03 bend.
It keeps the accepted route, dialogue, terrain, lighting, saves and settings.
The chimney-smoke line remains an unimplemented separate gap; no smoke, light,
mist, town expansion, river crossing or streaming policy is added here.

Work stays in the existing `.worktrees/horde-1.7-wp2-vertical-proof`, based on
`30b5e3ed8f85bc39a63db288081ecb0626bd9a8a`. That preceding test-only commit
corrects the old authored-view roster and retains its original CI failure.

## Assets and contact

The original numeric [recipe and source receipt](../../../assets/source/beyond_the_tomb/forest-road-clue-v1/README.md)
reproduce two closed GLBs without downloaded/generated-provider input. Source
masters remain outside packages. Runtime payload is exactly 9,808 bytes across
two GLBs and one manifest. Their 144 placed triangles share `MedievalWall02`,
the existing atlas and consolidated owner; no shader or material branch is added.
The licence record distinguishes original geometry from the existing CC0 maps.

The same transformed triangles join immutable nonwalkable contact. Existing
walkable and twenty STEP triangles remain unchanged. Final measured minimum
distance from a vertex to the actual route is 1.64405 m, preserving its 1.2 m
half-width plus 0.24 m capsule margin. Every authored base witness lies 30 to
281.644 mm below the actual terrain. Runtime admission enforces the burial and
clearance bounds and publishes geometry/contact atomically.

The initial placement failed six grounding assertions: two inner kerbs straddled
a terrain seam, with repeated base witnesses buried 313.859 and 324.236 mm.
Those failures are retained. Moving the pairs 0.5 m away from the bend fixes the
measured placement; the 300 mm limit is unchanged and now enforced in runtime.
An initial fixture compile typo (`id` instead of actual sample `triangle`) was
also corrected before any pass. No rejected geometry is silently admitted.

## Actual desktop RT inspection

The retained forward, reverse and lookout manifests record successful RT-produced
presentation, exact camera and staged route state, the original 960x540 output
and internal dimensions at 100%, Diagnostic/High shader hashes, and an NVIDIA
GeForce RTX 5050 Laptop GPU. All images were opened and inspected. Forward and
return views keep the dirt trail clear and show grounded masonry. The lookout
PNG is byte-identical to the matched control, preserving the village vista.

These are seeded accepted-death ascent/walk followed by frozen diagnostic views,
not a natural Keeper victory, earned save/load test, live listening or phone run.
Primary player/reward-lantern pixel counts and support/pose witnesses match the
control exactly. No framing acceptance check was removed.

Measured static RT resource deltas, identical in all three views:

| Resource | Control bytes | Candidate bytes | Added bytes |
| --- | ---: | ---: | ---: |
| Vertices | 39,956,288 | 39,974,720 | 18,432 |
| Indices | 6,377,292 | 6,379,020 | 1,728 |
| Production prop BLAS | 27,311,744 | 27,319,040 | 7,296 |

Material 3,840 B, primitive metadata 512 B, instance metadata 832 B, textures
274,027,796 B and nine descriptors remain unchanged. All 28 instance masks match.
The chapter owner still consolidates to eleven primitive rows; global fixed
capacities were not raised. These are resource receipts, not an FPS claim.

## Bounded matched cost comparison

Earlier baseline medians were 7.402/6.964 ms; initial candidate captures were
10.609/10.763 ms. That apparent regression prompted one control build omitting
only the owned three-line append. Its source hash and exact restoration are
recorded. The final candidate also records an initialization-only minimum-gap
report field; control does not call that function. Other production behavior,
shader bundle, settings, camera and staged state remain the same.

| View | Control frame wall median ms | Candidate ms | Change | Control GPU command mean ms | Candidate ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| F03 forward | 10.68205 | 10.50550 | -1.65% | 7.786672 | 8.119654 |
| F03 return | 10.64180 | 11.12390 | +4.53% | 8.005818 | 9.094936 |
| Lookout | 9.46300 | 8.98515 | -5.05% | 6.498160 | 5.897893 |

Each cell comes from one cold twelve-frame frozen capture. The new no-cue
control reproduces the earlier wall-time rise; it is not attributable to the
new geometry from these results. No matched median or GPU mean rises over 15%,
but the sample is too short to establish sustained cost or a performance win.
GPU readings bracket control at 42-43 C and candidate at 43 C; clocks/power
are retained in CSV, with idle/active P-states differing. No OS CPU utilisation,
thread profile, stable thermal plateau or Android result is claimed. JSON
comparison assertions can be rerun with `python compare.py` in this directory.

## Checks and remaining acceptance

Eight focused native Debug registrations pass, including actual terrain/contact,
Kit route traversal, five frozen chapter staging paths, provenance/duplicate
rejection, world ranges and unchanged primitive ceilings. The final forest
registration took 29.76 s; the separate open Debug 30-second full-route gate is
not relaxed or closed. The asset authoring verifier, exact regeneration and five
malformed-buffer negatives pass. Existing packaging admission/staging/archive
tests pass all 199 cases with their negative checks retained.

Android Debug builds all four ABIs, with 411 Java cases: 405 executed, zero
failures/errors and six existing host-only link skips. The actual APK passes the
closed runtime roster and byte/hash policy; the three new entries contain 9,808
uncompressed bytes and 2,999 compressed payload bytes. Their separate Windows-
style ZIP fixture is 3,396 bytes including headers, not a full distribution.

The local APK is 205,951,203 bytes, SHA-256
`b47df5177523b920ac7c8ede6c1a6a3c62685a9e596b1fe1a5b4151874911e5f`,
still `1.6.2-debug`/code 10. Relative to the retained 200,120,444-byte outdoor
APK, actual compressed entry payload grows 52,199 bytes; unrelated incremental
archive gaps grow from 5,410,380 to 11,198,934 bytes. Thus the whole-APK increase
of 5,830,759 bytes is not the masonry payload or a clean-release download estimate.
The retained layout audit separates entry bytes, alignment and signing gap;
no repack, clean release or publication was performed. Originals/quiet music and
this cue's source masters are absent. The first inventory helper used an incorrect
native library basename; inspection identified `libhorde_rt_probe_android.so`
and all four exact entries. That was a reporting-script error, not a build failure.

No phone operation, install, release or merge is involved. Owner visual acceptance at the
actual mobile scale, warmed device cost and the natural full journey remain
open. Existing music/Kit listening gates remain separate.

Audio/haptic manual revalidation required: **NO** for this slice; silent static
geometry does not change cues, event timing, listener/source routing or feedback.
