# Exact anatomical-mount phone validation and owner feedback

SM-S948B / Galaxy S26 Ultra, Android16/API36, Adreno840. Runtime source
`259bd26952867a3341b08a8a459dda4ff9b0a21e`, isolated
`com.samfa12.hordelanternrt.debug.viewmodel` ARM64 Debug.
APK SHA-256 `bd9b348ce351dc2e511e2f1c4f9aa7eaac050fcf7e1dfb3c05e7ffe98b991da9`.
Local immutable APK:
`C:/Dev/tmp/horde-anatomical-mount-android-20260927-c/HordeLanternRT-1.6.1-viewmodel-debug-arm64.apk`.

The APK is test-only. Initial `install -r` was rejected; explicit `install -r -t`
succeeded. On-device hash and the runner's pulled base APK match the above hash.
No data was cleared and the production package was not replaced. The dirty-tree
marker reflects retained scratch/documentation, not additional runtime edits.

`run-20260927-112636` passes all13 deterministic replay waypoints, four scene-only
captures and Home/resume, with strict ASTC and honest native RayTracingPipeline
presentation at explicit75%. All four captures report AnatomicalBody, the actual
dedicated-primary ownership predicate true, 60 Hz skinning and grip error within
15 mm. Shader bundle identity is retained in the state records; actual packaged
Mobile pipeline/compute SPIR-V validation passed separately.

| Checkpoint | Transport overflow | Shadow overflow |
| --- | ---: | ---: |
| Low lantern / look down145 | 0 | 0 |
| High lantern / look up146 | 8 | 0 |
| Low lantern / parry144 | 1 | 0 |
| Ordinary torch grips136 | 0 | 0 |

These counters keep physical-glass acceptance open. This run is Diagnostic
correctness/presentation evidence, not matched Shipping performance or backend
parity. A high/up completed-frame GPU sample was95.764425ms; it is not a sustained
distribution, a per-frame median, or an optimization claim.

## Owner feedback

After watching this test, the owner said: **"I think it looks really good!"**
Record this as positive owner-observed visual feedback for the new anatomical
mounting and framing on the exact installed build. Preserve that direction;
do not restart broad arm/roll/weight tuning. The automated replay and frozen
problem poses do not replace the still-required continuous attack/parry,
walking/look and high/low motion checks, detailed seam inspection, or deeper
look-down torso/legs acceptance. Those remain explicit next work.

The app was sent Home after validation to stop background GPU rendering.
Audio/haptic manual revalidation required: **YES for this profile**, because
shared held-item/light positions changed. No new owner audio claim was made.
