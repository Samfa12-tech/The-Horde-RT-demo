# Icon-faithful production reward lantern — 2026-08-27

## Art authority and generation lineage

- The owner-package icon reference is not present in this repository and is
  not claimed as a distributable file. Its former SHA-256 is retained only as
  an unverified owner-package locator in the tracked Task 7 review record.
  The surrounding architectural arch was explicitly excluded from the prop.
- Rejected preview candidate 1: `01a03f69-327a-7c6d-ac8d-ccdbb9fe13d7`,
  Meshy 7 Ultra, 25 credits.
- Accepted preview candidate 2: `01a03f6b-e99a-7ee2-9738-d6d236f89c2b`,
  Meshy 7 Ultra, 25 credits.
- Refine: `01a03f70-0499-7047-976b-daef45aa7684`, Meshy 7 PBR, 2K,
  `remove_lighting=true`, 10 credits.
- Remesh: `01a03f74-7501-7164-8113-53ba917fc66d`, triangle topology,
  target 9,000 triangles, bottom origin, 5 credits.
- Generation date: 2026-08-27 (Australia/Sydney).

Sanitized exact settings/task IDs and the accepted/rejected review record are
tracked in `assets/models/props/provenance/task-7/`.
They require an isolated compact blackened six-sided Gothic cage with a small
free ring, obvious hinge, steep pointed canopy, pointed lower finial, empty
pointed ogee/quatrefoil windows, and no modern camping form, pane fill, outer
arch, bracket, chain, hand, stand, floor, flame, glow, beams, text, logo, or
background. The refine prompt requests neutral forged iron and prohibits baked
light/highlights and orange/glowing textures.

## Review decisions

- Candidate 1 was rejected before texture/refine: boxy modern-camping family,
  fused opaque/yellow-looking panels, weak/mushy tracery, and unusable compact
  ring/hinge/finial hierarchy.
- Candidate 2 was accepted at geometry stage: compact six-sided open cage,
  steep canopy, pointed apertures and lower finial, readable top ring, and no
  surrounding arch/floor/beam contamination.
- The refined base colour contains no baked directional illumination. Its
  source central bulb/flame-like mass and all source pane fill were explicitly
  removed during deterministic processing.
- The 9K remesh retained the accepted broad silhouette but softened/collapsed
  tracery, so it is provenance only and was rejected for direct runtime use.
  No third geometry candidate was generated.
- Runtime geometry is a deterministic Blender 5.2 reauthor guided by candidate
  2 and the icon: exact `GripRing`, `Hinge`, `LanternBody`, `LanternGlass`,
  `Flame`, `Light`, and locally authored engine `FlameCore`; pointed canopy and
  finial; deliberate ogee/quatrefoil tracery; six separate closed 7 mm panes.

## Hashes

- Candidate 1 preview: `6e5c584a03e449052f3214ecfd918a55d5646fe5aa3aab900cbc16b39afc61be`
- Candidate 2 preview: `e612a8468125600eb37f2ed3485018beb989d5ac9f40e306a128aec9b917e59c`
- Refined PBR: `18f2395d39e13b682abf9350dce7cae2848c24d955d1f87af5ba53747b515443`
- 9K remesh: `4e5ed5bac69be626d6fc20e94597395819cf1b63e9aaf0caca145669a6218871`
- Clean ring/body source: `22cb880aee981d1e99147bf7e9e66b39d933ed96cb3acf39254a5a3638fdb113` / `3db6dfacf6cc871a404856c314e9d0616645c675f0b6f39e9a058caca15d5dd3`.
- Runtime ring/body: `d9b476a791739fc8951490e61682597d9e736b2f5c80595207aeb10bcae99359` / `ddc611bd90fc52cd2a68d944beb3ea8d17cb4e881b1c10f45395445622805f37`.

## Glass and emission

The runtime `LanternGlass` is unpainted and has no base-colour texture. It uses
KHR transmission 0.94, IOR 1.52, volume thickness 1.0, attenuation distance
1.8 m and colour `[0.94, 0.90, 0.78]`. All six components pass the repository's
edge-connected, directed/outward closed-manifold validator. Amber content comes
from the locally authored engine emissive core and active fire/light buffers,
not a Meshy emission flag or baked texture.

## Licence and attribution

No authenticated account-plan evidence was available. Distribution therefore
uses the conservative Meshy Free-plan CC BY 4.0 route. Attribution:
“Production Gothic reward lantern created with Meshy; runtime processing by
Samfa12/Codex.” Source/high GLBs and maps remain Git-LFS provenance and are
excluded from packages; only bounded runtime derivatives are distributed.
