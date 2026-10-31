# Whole-route forest: source candidate v1

Parent-authorized offline continuation after the four-tree grounding prototype.
This package is **not integrated scenery, a runtime LOD system, admitted collision,
or a performance result**. Production keeps its exact original 64 trees. This
layout replaces neither the originals nor any old trial roster; it must never be
combined with the overlapping north68/76 trials.

## Inputs and rights

The approved world concept, R2 forest/region sheets and coordinated JSON are
preserved under `docs/references/horde-1.7/d37f1638`. Original geometry, PBR maps
and editable centreline records come from `forest-tree-pair-v1`; both original
GLB and centreline SHA256 values are pinned in `author.py` and the receipt. No
third-party/provider asset, purchase, generation service or new texture is used.
The original owner rights remain; no blanket MIT, CC0 or other asset licence is
assigned. See the root `ASSET_LICENSES.md` entry.

The fixed map transform `X=E+78.3, Z=126.2-N` reproduces all six protected route
points and the 76.264 m path. Tree circles on the original map are massing intent,
not locked counts. Terrain support comes from the exact imported r23 GLB, not
planning heights or route-ribbon extrapolation. The frozen original64 roster and
all original model/image bytes remain unchanged.
The two editable placement-JSON/native-rock-C++ text input identities use explicit
UTF-8 LF-normalized SHA256, so Git's normal CRLF checkout does not change their
identity. Frozen reference/archive inputs and all GLB/PNG/generated output checks
remain byte-exact. Existing input files are never rewritten to enforce this.

## Authored outputs

- Original64 near placements retain their exact identity and existing collision.
- 96 middle trees in eight stands use new branch-supported 3D crowns: pine 1,474
  triangles, alder 1,390. They are eligible only >=12 m from the complete proposed
  walking envelope, not merely far from the current camera.
- 320 far trees in fifteen stands use simple closed opaque silhouettes: pine164,
  alder180 triangles. Their nearest possible player distance is >=38 m.
- One original 2.4 m three-rail timber boundary module has60 triangles. It reuses
  the existing owned bark material/maps. Three narrow rail openings are visible;
  actual capsule/crouch/dodge containment is still a native acceptance task.

All assets are local-metre Y-up, finite, outward closed triangle volumes with UV0,
normals and mapped tangents. Every embedded PNG payload is byte-identical to its
original. Materials reuse the existing four families; this does not itself prove
final atlas intake, GPU residency, appearance or suitability at the assigned range.

`composition.json` contains480 unique stable records, conservative town/garden and
river exclusions, church/tavern view windows, untouched clear path, reunion/lookout
pockets and a larger F01 pocket preserving graves, slab/shaft and rope access.
The additional roots have3,000+ actual terrain/root-tip probes and <=0.65 m fitted
burial. These offline probes are not native continuous grounding acceptance.

## Boundaries and collision

The source-only closed boundary follows the walking envelope and bends around
existing reachable trunks.196 terrain-bound runs provide a **visible fence
fallback** for every unproven natural gap;77 runs are flagged to inspect actual
existing terrain banks. These are assessment candidates, not certified impassable
slopes. Prefer grounded bank/rock runs and short fences in the final composition,
not a continuous fence enclosing a forest or an arbitrary tall terrain wall.
The12 existing rock placements are derived read-only from their native authoring
roster and linked to nearby boundary runs, not duplicated or moved. Their grounded
mesh/collision authority stays in the existing native owner; no single rock is
assumed to close a gap without the later contact proof.

Module endpoints record actual ground height and rail conformance delta. Native
integration must keep posts upright, conform rails, test exact rendered wood/rock/
bank collision, remove self-intersection/short-run art issues and prove the whole
boundary under walk/run/crouch/dodge and return. No invisible sheet is admitted.
Existing original64 collision is never removed. New off-corridor trunks deliberately
have no proposed collision **only after** the visible boundary really constrains
access; every relevant admission flag is false at this checkpoint.

## Budget and representation

The exact composition has515,040 tree triangles, adding191,776 to323,264 originals.
Its native-format added visual payload is10,949,376 B under the current64-byte
vertex/32-bit-index convention. The conservative authored bound is26,848,640 B
at two vertices per triangle. The196 fallback panels add11,760 triangles and
642,880 raw geometry bytes. Source GLBs duplicate embedded images for standalone
inspection; final runtime should reuse existing texture families, not upload
one image set per tree. Actual memory, AS/scratch, copies, transition peaks and
frame/dispatch cost remain unknown.

The intended first native trial is four-family static batching with fixed detail
roles across the bounded corridor, not a full-resolution bake for every tree.
No runtime LOD switching, alpha billboard support, renderer refactor, wind, new
lights, global shared64 enablement or capacity change is implemented here. The
existing shared-BLAS experiment remains separately gated. The generated source
plan has no automatic runtime consumer and normal defaults do not change.

Old triangle/whole-instance retention certificates cannot be applied to this new
full source. New packed-source provenance, retained shadow/reflection/water/light
contributors, safe backtracking, restoration and recreation must be proven during
native integration. Initially retain the complete candidate rather than weaken a
certificate or label residency as streaming.

## Reproduce and inspect

From the worktree root, standard Python only:

```text
python assets/models/world/source/forest-surroundings-v1/author.py --check
python tests/ForestSurroundingsSourceTests.py
```

`--write` deliberately regenerates only this draft package's fixed eight outputs.
It never writes original sources, shipping rosters, capacities or gameplay state.
The native static-GLB test's `--inspect-candidate GLB MANIFEST` option supplies
separate actual native importer evidence; it starts no Vulkan device or game.

Optional `preview.py` uses installed Blender's CPU Cycles in background, with
neutral studio lighting, no exporter and no foreground. `layout_preview.py` uses
optional Pillow to illustrate the authoring plan. Their explicit outputs must be
under ignored reports. Blender previews/maps are not Horde RT gameplay captures.
No `game-dev` CLI is installed; none was installed or substituted with a provider.

Next gate: source/native integration and actual moving F01/F02/bend/F03/F04 views,
upward/side/reverse views, landmark preservation, lifecycle/recreation and resource
measurements. Follow with coordinated Android correctness/cost evidence. This
package does not close the owner's surrounding-forest goal. Performance remains
ongoing optimisation rather than a fixed-FPS release veto.
