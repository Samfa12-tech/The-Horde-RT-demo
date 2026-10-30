# Kit partial-angle contact gate and large-turn preview

**Visual follow-up:** the [timestamped reassessment](VISUAL_REASSESSMENT.md)
finds no objectionable visible defect at gameplay distance and withdraws the
recommendation for another 22.5-degree pair. It recommends the existing recipe
for a bounded shared-state trial, retaining contact measurements and pacing
limits. The initial measurement disposition below is historical; it is not a
demonstrated visual failure or a new zero-slip requirement.

24 October 2026, based on `3fd5ef6c` plus the source identities in
[receipt.json](receipt.json). **Gate A remains unpassed/unadmitted. The shared
production turn/start controller was not implemented.** Original production
bindings, seven clips, source/runtime assets, route/tether/speed and saves remain
unchanged. This checkpoint backs up the [scope plan](../../superpowers/plans/2026-10-24-kit-shared-turn-start-scope.md)
and the finite prerequisite study only.

## Result and disposition

Matched joint/yaw amplitude blending is structurally valid, reaches its endpoints
and produces restrained partial poses. It does **not establish preserved planted
contact**. At 22.5 degrees, both directions show more contact-point movement than
the full 45-degree control across all measured height bands. The 2-degree take
also leaves the nominal swing sole extremely close to the normalized floor.
These are reasons to withhold the blanket admission claim, not a newly invented
zero-slip tolerance or a claim of an obvious visual defect.

The actual RT night views do not show a gross skid, inversion or pose pop. They
also do not resolve the small contact regression strongly enough to accept it.
No additional runtime workaround or numerical-perfectness loop follows this
result. The working `contactAdmitted=false` remains explicit.

## Contact method and retained measurement correction

The optional existing intake executable now accepts:

```text
horde_rt_kit_source_intake_tests.exe --turn-contact <owned-candidate.glb> <fresh-output-folder>
```

It uses the actual `SkinnedPbrActor` joint blend and floor/bounds packing, original
Idle zero and the unchanged authored left/right clips. Joint amplitude and the
exported model-yaw curve both use `degrees/45`; key interpolation matches the
owned 46-key yaw profile. There is one yaw application and no geometry, bind,
weight or animation edit. Ten cases include both directions at 0, 2, 10, 22.5 and
45 degrees, each at 120 Hz / 91 times. The same 39 left / 38 right native sole
rows are selected from the original pure leg/boot weights and lowest 6 mm.
All-vertex endpoints agree with Idle zero within **0.000732 mm**.

The first receipt measured only the authored support windows. Its
[original summary](prior-declared-support-only.json) is retained. That statistic
omits nominal swing even when a partial-amplitude sole touches the floor. It must
not be called a maximum over every physically contacting point. At 2 degrees,
some omitted swing samples have sole minimum Y=0; maximum nominal swing-foot
minimum height is only about **1.114 mm**.

The corrected [measurement](measurement.json) and [all samples](sole-samples.csv)
retain reference offsets during swing and add material-point contact runs.
Each selected vertex anchors X/Z on entering a height band, accumulates
displacement/path over consecutive samples and resets on separation. Three
**measurement sensitivity bands**, 1 micrometre / 0.1 mm / 1 mm, are reported;
they are not slip admission tolerances. These are sampled, floor-normalized
model-plane measurements, not an exact contact solver or per-boot intersection
with the game's sloping terrain. Contact membership can change between samples.

Worst values across the two directions, millimetres:

| Turn | Max drift in declared support windows | Max mean support-point drift | Max same-point run displacement: 1 um / 0.1 mm / 1 mm bands |
| --- | ---: | ---: | ---: |
| 0 deg control | 0 | 0 | 0 / 0 / 0 |
| 2 deg | 0.939 | 0.808 | 0.422 / 0.923 / 3.291 |
| 10 deg | 3.993 | 3.288 | 1.675 / 1.863 / 5.386 |
| 22.5 deg | 6.504 | 4.742 | 2.998 / 3.236 / 8.433 |
| 45 deg control | 6.856 | 0.105 | 2.318 / 2.360 / 4.915 |

Maximum declared-support drift alone makes partials look better; the mean and
contact-run measurements preserve the additional coherent movement instead of
hiding it behind the source take's sole deformation. Reference offsets in swing
use the initial pose and include intentional foot travel; only the explicitly
labelled support summaries treat that reference as a plant.

## Actual RT visuals and authored-speed previews

- [Partial contact at 1x authored time](partial-angles-contact-authored-1x.mp4):
  all eight requested cases side by side, 2x nearest-neighbour crops of original
  RT pixels. No relighting, interpolation or generated replacement imagery.
  Nineteen source frames at 24 Hz represent 0.750 seconds of motion; 0.5-second
  lead/end holds make the encoded 43-frame video **1.791667 seconds**.
- [Left sole sheet](left-sole-contact-sheet.png) and
  [right sole sheet](right-sole-contact-sheet.png): labelled original crops at
  five phases, with the same 2x nearest-neighbour scale.
- [180-degree sequence at 1x authored time](four-45-turns-180deg-authored-1x.mp4):
  four unchanged 0.75-second left turns, with a fixed supported root and no added
  gap. Seventy-three actual RT samples span **3.000 authored seconds** at 24 Hz;
  the terminal sample plus 0.5-second lead/end holds give 97 encoded frames /
  **4.041667 seconds**. One bottom row is padded for even H.264 dimensions.
- [Large-turn frame sheet](large-turn-reviewed-frames.png): decoded source views
  every 0.375 authored seconds, not a continuous-video or owner-feel pass.

The playback clock is the declared animation timeline, **not observed engine
FPS, scanout or a live gameplay recording**. The candidate is isolated inspection
playback, not today's production turning behavior. Original body/boot frames,
including dense phases 0.444 and 0.556, were inspected. Real-time pacing and final
motion comfort still require owner review; no live video playback/feel acceptance
is claimed from inspecting decoded frames. The long turn visibly consists of
four separate step cycles. Do not silently adopt its three-second delay, change
the tether, or raise the speed ceiling to conceal it.

## Native/RT checks and custody

The original sampled+large run passed **148 checks / 126 RT calls** in 28.98
wrapper wall seconds. The supplemental dense run passed **187 checks / 165 RT
calls** in 34.53 seconds; all 40 previously sampled partial PNGs remain
byte-identical. These checks establish completed RT presentation, selected
inspection pose, unchanged loaded story/records and cleanup, **not contact
admission**. Both stay within the unchanged 90-second / 180-frame guards.
GPU allocation inventories remain equal across each run's inspection frames.
Native Debug builds and the ordinary original-asset intake CTest pass (1/1,
7.16 seconds). No shader or shared gameplay source changed.

All owned processes exited; hidden owners remained unshown and external
foreground stayed unchanged. Staged settings, source/candidate assets, isolated
autosaves/manifests and all 151 parked owner-file hashes are preserved. The worker
remains OFF. The two complete RT receipts, all PNGs and source/build logs remain
under ignored `reports/kit-partial-contact-20261024/` and named isolated fixture
folders; compact identities and quantitative evidence are backed up here.
The dense wrapper retained an overbroad copied class string mentioning the large
turn; its native manifest correctly identifies dense-only frames and no large
sequence. The original record is retained rather than rewritten or rerun.

## Retained earlier authored-correction proposal

This proposal followed the initial measurement-only disposition. The visual
reassessment above does **not** currently justify starting it. Keep the original
two candidate clips for the next trial; the following brief is retained as
history rather than silently erased.

Author a separate **left/right 22.5-degree take** from
`assets/models/characters/source/kit-step-turn-proof-v1/kit-step-turn-cleanup.blend`
and the existing `author_turn.py` recipe. Recompute the actual angular foot goals
and their fixed support positions; author/review swing clearance independently
of a global yaw-amplitude scale, then bake the same-rig joint motion. Preserve
Idle-zero endpoints, geometry, binds, weights and the original 45-degree files.
This targets the demonstrated midpoint problem without adding runtime IK,
foot-lock corrections, a general mixer or a large animation library.

That derivative is a proposal, **not created or guaranteed to fix the 2/10-degree
cases**. Recheck those explicitly before selecting any small-angle blend policy;
do not skip 2 degrees by changing a threshold merely to pass. Large-angle pacing
is a separate review question, not justification to bundle another clip family
into this correction. Keep the shared state and production package selections
unchanged until this prerequisite is accepted.

Audio/haptic manual revalidation required: **NO** for this test-only study.
No phone, foreground activation, paid generation, merge or release occurred.
