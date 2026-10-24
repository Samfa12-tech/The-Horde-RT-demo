# Player lateral gait - unadmitted authoring study

Editable same-rig Blender source, **not a runtime asset or accepted animation**.
The source mechanism gap is real: the current player rig binds only `Idle` and
`Walking`; lateral motion still samples forward walking. This study does not
replace either binding, change movement speed or introduce a new locomotion API.

`player-lateral-knee-proof.blend` contains left/right lateral crossover takes.
Geometry, weights, bind transforms and scale remain unchanged. Original Idle and
Walking source data remain in the file. The root-speed contact calculation uses
1.9 m/s; the editable scene itself is an in-place authoring preview, not moving
gameplay. Preview matte materials and a studio floor are inspection aids.

Source: `assets/models/player/runtime/gothic-traveller-lod0.runtime.glb`, SHA256
`46a88dac9dd569117be5a17a35540fecf3e0e6026d3ad404dd5fdbc85a9ad54f`.
Provenance and CC BY 4.0 attribution remain those of the owned Gothic traveller
and gauntlets in [METADATA.md](../meshy-2026-08-26-gothic-traveller-candidate-2/METADATA.md)
and [ASSET_LICENSES.md](../../../../../ASSET_LICENSES.md). No paid generation,
new download, retargeted third-party rig or licensing change occurred.

The first near-straight take exceeded available leg reach by 94.7/95.7 mm.
One 120 mm hip-drop correction removes required stretch, but the inspected
crossing poses remain cramped. Adjacent planted-sole drift reaches 5.1/4.6 mm,
and sole minima reach -5.7/-5.5 mm. These metrics are evidence, not new universal
tolerances. Do not mark the candidate visually accepted from zero stretch.
No native clip was exported and no runtime/RT join, reflection, shadow, pause,
save or performance acceptance is claimed. The original player remains active.

Reproduction uses the existing Blender 5.2 background workflow, from the
repository root, with a fresh destination:

```text
blender --background --factory-startup --threads 1 --python-exit-code 1
  --python assets/models/player/source/lateral-gait-study-r1/author_strafe.py
  -- --output reports/<new-lateral-proof> --knee-cleanup
```

Omit `--knee-cleanup` to reproduce the rejected first pose. The script refuses an
existing destination. Optional file thumbnails are disabled only in that Blender
process; preferences are not saved. The initial source save succeeded while an
optional global-thumbnail write failed; that path was not retried.

The [evidence](../../../../../docs/evidence/2026-10-24-player-lateral-study/README.md)
retains both failures and measurements. This source folder is not registered in
any runtime/package manifest. A useful next attempt needs a deliberate lateral
gait and weight-transfer reference, not more speed or clip-phase tweaks to this
crossover family. Production import would still need unchanged static data,
native loader/contact validation and actual in-game views.
