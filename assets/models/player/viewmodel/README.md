# Viewmodel candidate

This directory stages the offline viewmodel candidate for CPU admission checks only. It is not packaged, connected to the RT renderer, or accepted for normal gameplay or owner review.

## Candidate and provenance

- Runtime: `runtime/gothic-traveller-viewmodel.runtime.glb`
- SHA-256: `1e3b041ee7aa896a84fe462c182f6012d026b4a577b2d4ddf59d764b823f2538`
- Role: explicit `Viewmodel`, with `ViewmodelSleeves` and `ViewmodelGauntlets` as primary-only parts.
- Source lineage: derivative of the existing licensed Meshy player/gauntlet inputs, using Blender 5.2.0 LTS build `fbe6228777e7` and the existing player processing route.
- Processing was pinned to `--threads 1`; offline repeat candidates B and C were byte-identical.
- No paid generation was performed for this candidate.
- `runtime/viewmodel-processing.json` records the source hash, named primitive triangle counts, ownership, animation authority, and offline-only status.

The existing `ASSET_LICENSES.md` CC BY 4.0 attribution covers the Meshy player and viewmodel-gauntlet inputs. This staged candidate does not alter licence statements.

## Validation boundary

The focused host test invokes `horde_rt_skinned_character_smoke --validate-viewmodel-admission` with this runtime, its manifest, and the world-player GLB. That proves bounded manifest/geometry admission and authored rig/socket agreement only. It does not prove GPU ownership, grip/pitch/motion composition, RT presentation, Android packaging, device behaviour, or owner acceptance.

Regenerate into a new directory from the repository root:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --threads 1 --python-exit-code 1 --python tools/process-player-viewmodel-runtime.py -- C:/Dev/tmp/horde-viewmodel-new
```

The processor refuses existing output directories and verifies its full-world
reference against the admitted world runtime before deriving arms. It preserves
all original sources. Export failure must propagate a nonzero exit code; the
`--python-exit-code 1` argument is required.
