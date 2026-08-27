# Production Gothic reward chest — 2026-08-27

## Generation lineage

- Tool route: repository `tools/meshy-safe-api.mjs`, official current Meshy API.
- Preview: `01a03f69-2b15-7c6b-b17f-de4741cdafba`, Meshy 7 Ultra, triangle topology, no automatic remesh, GLB, 25 credits.
- Refine: `01a03f6c-ba91-7f43-a42a-10d2f09f9f09`, Meshy 7 PBR, 2K, `remove_lighting=true`, GLB, 10 credits.
- Remesh: `01a03f74-72ac-7e62-8b58-cd906ddc758a`, triangle topology, target 7,000 triangles, bottom origin, GLB, 5 credits.
- Generation date: 2026-08-27 (Australia/Sydney).

Exact request payloads are retained in
`.superpowers/sdd/2026-08-26-fire-pbr-reward-lantern-player-upgrade/evidence/task-7/meshy/`.
The preview prompt requested an isolated compact dark-oak Gothic reliquary chest,
blackened iron, pointed/quatrefoil motifs, separate base/lid/latch, rear hinges,
empty interior/socket space, and explicitly excluded lantern, floor, coins,
weapons, chains, flame/glow, particles, text, logo, and background. The refine
prompt requested neutral aged-oak/forged-iron PBR with no baked directional
illumination or emission.

## Review decisions

- Preview accepted after neutral six-view silhouette review: compact open
  reliquary form, readable lid/base, latch and rear hinge family, no prohibited
  scene contamination.
- Refined PBR accepted as neutral source reference; direct base-colour inspection
  found no baked torchlight, glow, or directional shadow.
- The 7K remesh preserved the broad silhouette but collapsed ornamental surfaces
  into rocky folds, so it is provenance only and was rejected for direct runtime
  use. No second chest geometry candidate was needed.
- Runtime geometry is a deterministic Blender 5.2 rigid reauthor guided by the
  accepted silhouette: exact `ChestBase`, `ChestLid`, `Latch`, and
  `LanternSocket`; rear-hinge local lid origin; no skinning or hidden interior
  fill. Processing is reproducible with `tools/process-production-gothic-props.py`.

## Hashes

- `chest-meshy7-ultra-preview.glb`: `9ae9a0591b31b269dd75d1cbcaf1615d9b26c9001f0d4d6454ffa5772b82e5f9`
- `chest-meshy7-ultra-refined-pbr.glb`: `50a78e025c4e9828394a3dd4966ec66601f9b49ce1e808cf90af71216636c700`
- `chest-meshy-remesh-7k.glb`: `eb70e139f5f776dc7204b9fff498bf9cdc7c75f275d65053d73a597b6908c5ab`
- `gothic-chest-base-cleaned-source.glb`: `a312352d5f68b60ce41d45504d4ac06a919dc422c1c77f6cb631e60874e5ae2a`
- `gothic-chest-lid-cleaned-source.glb`: `1c2d0a57608bfe30003777a0307516086a8b0bb056440b6da67020f51c0ab917`
- Runtime base/lid: `32417bc40fdb8e9467c32d9f29f811efdd6b35a1c717e3f6eb1b628112488f89` / `24d771eab7d598192cf520d0a382e03486c15153b52e45ee6421a32ead3b1b25`.

## Licence and attribution

No authenticated account-plan evidence was available. Distribution therefore
uses the conservative Meshy Free-plan CC BY 4.0 route. Attribution:
“Production Gothic reward chest created with Meshy; runtime processing by
Samfa12/Codex.” Source/high GLBs and maps remain Git-LFS provenance and are
excluded from packages; only bounded runtime derivatives are distributed.
