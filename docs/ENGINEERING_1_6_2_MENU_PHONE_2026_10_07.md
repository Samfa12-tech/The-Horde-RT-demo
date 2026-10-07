# 1.6.2 Entry, phone menu and Glass timing checkpoint

7 October 2026. Authorized finishing work on existing draft PR18; no release.
Private raw logs, screenshots, preferences and device serial remain outside Git
in the existing `task-4` parent. Earlier completed `1334cc9` device/persistence
results remain unchanged and are not replayed or relabelled.

## Exact foundation

Source `b3ea5ed01fc7a8680fc7ff55c78e5f79c0cf1aa3`, tree
`014403ac2cff79434ee83dfb602a35609e019d23`, has an empty runtime diff at seal.
Only tracker/release prose and a semantically empty line-ending file were dirty.
This foundation is not the final review candidate.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Four-ABI development Debug APK | 139693605 | `7b566fd1036ba4c52c62a5519ffa933ab1ac61dcb7b98f916f018144b5405b3f` |
| Windows Debug executable | 11163648 | `689724fff65fc29bfc0d12cb77c795d9ee76e03f43fc9026199ed56ed719b4e9` |

Windows Debug build, Android four-ABI assembly, 172 Java tests in 29 classes
and lint pass (zero errors, 55 warnings). The affected native selection passes
12/12 in 22.64 s, covering Entry handoff, cadence, held transitions, combat trace,
Entry scene/preview adapter, simulation timing/gameplay, Windows music focus,
mailbox, controller policy and combat smoke. The previously corrected full Rag
clearance evidence is preserved separately rather than rerun unchanged.

Push/PR foundation runs `37589001344` / `37589006869` each pass all six jobs.
Current phone plaque commit `013770319d06cb05d9c7714ccb976453f21e926b` also
passes all 12 checks: push `37590266663`, PR `37590274408`.

The foundation APK was installed with existing development-app data preserved
on allocated SM-S948B / Android 16. The installed APK pullback matches exactly.
No production app, data clear, production signing or release was used.

## Appearance and menu state

The owner approves the Windows central hanging-lantern composition and asks for
a little more emitted light. Strength .90 to .99 passed actual landscape and
portrait Windows RT inspections, Graphics no-Use Back, saved-settings retention
and Play handoff; exact earlier dirty executable/captures are retained in the
active tracker. This approves direction, not phone cost or motion/audio feel.

The initial foundation phone menu **fails** appearance inspection: a narrow
parent modal wraps title and Settings at the existing 1.7 font scale. Preserve
`phone-menu-b3ea5ed0-20261007/01-entry.png` as failed evidence. No system font
or density setting was changed to hide that defect.

Commit `01377031` places native Settings, More and Play on separate plaques over
the genuine RT scene, with measured text widths and focus/touch targets. Actual
portrait inspection passes readable labels and separate controls; owner review
is pending. Image `phone-menu-01377031-20261007/01-entry-portrait.png` has current
generation 1 / Entry profile 2 / successful RT presentation. Landscape was not
tested: inspection found the manifest's historical `sensorPortrait` lock. A
temporary device rotation test was restored to original `lock 0`.

This Java-only inspection package is explicitly mixed-source: Java `01377031`,
native `b3ea5ed0`. Native compilation tasks were excluded while physical sword
work was dirty; all four packaged native library hashes match the sealed
foundation. APK SHA-256
`8bb4b96451f2e6db6a5302060ee84549b38b5defc2b2e766d9838e003f80e2c7`,
139693605 bytes. It is not an immutable integrated candidate. Its installed
pullback matches that exact APK hash. Source/receipt are retained under the named
inspection directory. System font scale remains 1.7 and density override 560.

At this checkpoint Graphics still has excessive always-visible explanations;
preview actions also extend outside the visible row. UIAutomator could not reach
idle during live preview; failed dumps are retained. The older pulled
`05-preview-pending-ui.xml` is stale and **not valid evidence** for that preview.
Actual screenshots/logs remain useful. Concise groups, optional details and
persistent discoverable Use/Keep/Restore controls are the next UI correction.

## Actual Glass baseline

Same allocated phone, RayTracingPipeline backend, output 1440 x 2980, traced
50% at 720 x 1490; saved Mobile water/fire, Glass On, Current shadows, cap30,
Mist On. This is a saved/custom tuple, not a fresh-default change.

| Operation | Total CPU wall ms | Pipeline creation ms | Other relevant stages |
| --- | ---: | ---: | --- |
| Cold Entry, Glass On | 14139.468 initialization | 14084.374 | Opaque 2915.948, generic 11168.039 |
| Entry to preview, Glass On comparison serial 2 | 15918.424 request completion | 15581.840 | Initialization 15755.992; request owner wait 15768.188 |
| Preview Glass Off comparison serial 3 | 16694.868 request completion | 16174.583 | Owner wait 113.775; idle 173.496; destroy 5.672; initialize 16357.363; first-render completion 44.544 |

All baseline pipeline calls used null pipeline caches. Off still initializes
both admitted bundle pipelines. SBT/geometry/resource stages are comparatively
small; measured pipeline creation accounts for about 97% of Off request wall
time. This warrants testing one device-owned memory cache. It does not prove
cache benefit, justify a renderer rewrite or weaken acknowledgements.

Glass Off reached current preview generation 1/profile 1. `exact_ack=0` is
correct for a comparison, which is not Use/save. A return-On UI attempt did not
produce a new confirmed serial and is a remaining measurement gap. Native exact
ACK and the normal 15-second confirmation after ACK remain unchanged. The prior
persistence checks are not failures.

Saved/custom preference keys remained unchanged; only interrupted-edit recovery
metadata changed. Relaunch from the confirmed tuple presented current Entry and
cleared the pending marker. USB-powered/battery-98% context is recorded, not
reliable sustained power/thermal evidence. No reciprocal GPU-time or single
median claim of displayed/sustained FPS is made.

## Open gates

After this phone inspection, physical sword Hips/Grip composition was committed
as `5a0806ba` (production activation remains off): transition/actual-rig
socket/gameplay CTests pass 3/3; direct death interruption and copied-state
recovery pass 1/1. There is no scabbard asset or moving/device acceptance yet.
The borrowed device-owned memory pipeline cache is `f5835b5d`; Windows build
and preflight/pipeline-policy CTests pass 3/3 in 135.13 s. A build invocation
incorrectly requested the two script-only test names as build targets; it failed
with MSB1009 and was corrected without changing assertions. These host checks
prove the ownership seam, not a Vulkan cache hit or phone latency improvement.
Both checked slices are pushed. At `f5835b5d`, all 12 aggregate CI checks pass,
six each in push `37592083097` and PR `37592090750`. Four-ABI Android native
compilation also passes in 54 s; no cache-enabled phone package has yet been
installed or timed at this checkpoint.

Corrected phone appearance, landscape/session/surface recovery, Entry/Play/Home
cost, actual foreground pause work/thermal reduction, cache benefit, moving
equipment/contact/touch, integrated 50/40/33 comparison and owner decisions
remain open. Final immutable packages/aggregate CI, Eric's independent audit
and owner release approval are separate. Periodic checked commits/pushes back
up work to existing PR18; they are not releases.
