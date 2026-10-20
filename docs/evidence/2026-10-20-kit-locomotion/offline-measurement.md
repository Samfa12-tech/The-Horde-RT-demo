# Warden walk: offline cadence and transition audit

Source HEAD `d2d24e3d`. This is independent GLB analysis in Python double
precision, following the native TRS/Nlerp/palette rules. No game, RT, device,
production source, test, route, save or preference was executed or modified.

Runtime GLB SHA256:
`1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713`.

Reproduce from this worktree (Python standard library only):

```
python reports/kit-walk-audit-20261020/measure_walk.py --glb assets/models/characters/runtime/kit-warden-v1/kit-warden-lod0.runtime.glb --output reports/kit-walk-audit-20261020/walk-480.json --samples 480
```

Repeat with 120/240 for the retained lower-density JSON/CSV results. The script
permits output only in its own ignored reports folder. It reads immutable GLB
bytes, including the complete node hierarchy, all four skin weights and inverse
binds; it does not reapply the Armature's 0.01 root scale. The native reader
currently interpolates TRS regardless of source LINEAR/STEP labels; the script
matches that behavior, rather than silently using a different glTF sampler.

## Measurements

- Walk cycle: **0.833333313 s**; idle cycle: **2.0 s**. One complete walk cycle
  contains both steps, giving **144 steps/min** at current 1x playback.
- Boot subsets: **1046 left / 1025 right** vertices. Bind Y < 0.25 m and every
  nontrivial skin weight belongs to that side's UpLeg/Leg/Foot/ToeBase branch.
- Contact proxy: a boot's minimum vertex Y is within 10/20/30 mm of the lowest
  semantic boot. Rearward boot-centroid Z is fitted over the middle 70% of each
  sustained low interval. Brief forward-travel low intervals are retained in
  JSON but excluded from the natural-ground-speed aggregate.
- At 240 samples / 20 mm: sustained left interval **0.205–0.694 s**, fitted
  backward speed **1.9640 m/s**, fit RMS **2.35 mm**; right interval
  **0.583–1.097 s** wrapping the cycle, **1.7816 m/s**, RMS **7.12 mm**.
- Across all nine sample-count/threshold combinations: median of the two
  sustained stance speeds **1.87231–1.87325 m/s**, inferred complete-cycle
  stride **1.56026–1.56104 m**. Matching unchanged 1.8 m/s translation implies
  visual playback **0.96090–0.96138x** (about **138.4 steps/min**).
- Relative boot lifts above the lowest boot: **111.1 mm left / 112.8 mm right**
  at 480 samples. Raw ungrounded minimum-sole Y ranges (124/111 mm) include hip
  bob and should not be mislabeled as the floor-repacked lifts.
- Hips global ranges: **3.27 mm X / 79.89 mm Y / 10.32 mm Z**; Armature global
  translation is constant. There is no forward locomotion root to cancel the
  stance sweep if the same clip is used with a stationary route root.
- Idle-zero to walk-zero raw node jumps: left ankle **115.2 mm**, left toe
  **117.1 mm**, right ankle **50.6 mm**, right toe **45.7 mm**, pelvis **25.1 mm**.
  These are ungrounded node deltas; actual rendered boot displacement should
  be confirmed natively after the actor's per-pose floor normalization.

## Recommendation and limits

The **1.561 m** inference is numerically stable enough to propose a small
**0.96x visual-only cadence candidate**, with route speed and root/save timing
unchanged. Do not claim 0.961 is a precise biomechanical ground truth: centroid
fit, roll and left/right asymmetry remain systematic uncertainties. Native
NodeTransforms/skin confirmation and an actual native moving comparison should
precede adoption. Current 1x is already a near-match, not a large speed defect.

Stop resetting the visual phase at every transient walking edge. Blend local
joint TRS over a short bounded transition using one skin pass, preserve exact
blend endpoints, and keep visual phase/transition state transient. Measure the
actual per-frame boot jump through start/stop/waypoint/wait/pause/restore. A
speed/distance-derived phase is useful only if driven by accepted route travel,
not attempted movement against an obstacle.

Reject straight walk as a claimed stationary stepping-turn fix. It has useful
foot lifts, but a stationary root leaves the apparent stance foot sweeping
backwards roughly **1.8–2.0 m/s**. Adding yaw cannot cancel both sagittal stance
sweep and lateral pivot motion. Real foot locking needs an admitted turn clip,
root compensation or IK and separate contact validation. Preserve current yaw
policy in this bounded slice; do not relabel a walk-at-zero-speed as a planted
turn. Later bounded head/upper-body attention or a separately validated turn
route may improve stationary behavior.

These results do not certify native output bytes, terrain contact, visual
quality, perceived animation, moving gameplay or performance. Native rendering,
device validation and any implementation belong to the root integration lane.
