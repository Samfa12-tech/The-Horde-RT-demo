# Kit same-rig step-turn authoring proof — unadmitted

Editable `kit-step-turn-proof.blend` contains two independent named scenes:
LEFT and RIGHT, each a 45-degree / 0.75-second candidate at 60 fps (frames 1–46).
These are authoring choices, not fixed gameplay targets. The scene's paired
armature and `PROOF ONLY root yaw` actions belong together. Ground, camera,
preview colours and root-yaw wrapper are authoring helpers, not runtime assets.

Derived from the [untouched owner-approved master](../kit-warden-v1/README.md),
SHA-256 `47e249840ed9dd438da386137257050a5bd463e7fe54bf94873e1fd21715835e`.
The original 24-joint skin, geometry, weights, imported bone bind matrices and
Armature scale are preserved. Blender imports glTF's Y-up into its Z-up authoring
space; this is not a newly exported GLB or proof of native byte correspondence.
Original source/runtime GLBs and the pinned `prepare-kit-runtime.py` guard remain
unchanged. Existing source clips stay in the editable file.

See [previews, measured limitations and integration boundary](../../../../../docs/evidence/2026-10-23-kit-step-turn-proof/README.md).
Maximum support-sole drift is still **12.1 mm** in the reopened baked take;
the result is not claimed fully planted or production-ready.

Reproduce into a fresh ignored directory, from repository root:

```powershell
blender --background --factory-startup --python assets/models/characters/source/kit-step-turn-proof-v1/author_turn.py -- --output reports/kit-turn-proof-reproduce
blender --background reports/kit-turn-proof-reproduce/kit-step-turn-proof.blend --python assets/models/characters/source/kit-step-turn-proof-v1/verify_preview.py -- --output reports/kit-turn-proof-render
```

The recipes refuse existing output directories. The second command samples the
saved takes at 120 Hz and renders 46 frames per direction using Blender Workbench.
Those are explicitly **offline studio previews**, not game RT screenshots,
runtime acceptance, phone evidence or performance measurements.

No new paid generation, retargeted skeleton, topology edit, weight edit, runtime
binding or package roster change. Source provenance remains the existing Meshy
entitlement confirmation in root `ASSET_LICENSES.md`; not MIT/CC0.
