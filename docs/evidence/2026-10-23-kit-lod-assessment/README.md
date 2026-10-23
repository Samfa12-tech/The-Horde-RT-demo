# Independent Kit lower-detail assessment

Follow-up: the [single approved candidate review](../2026-10-23-kit-lod-candidate/README.md)
now supersedes this feasibility snapshot. Its admission review compares new risks
to the inherited source baseline; the proposed blanket static rejection wording
below is historical, not a requirement to reject inherited Meshy defects.

**Read-only feasibility, no Kit derivative generated or admitted.** This follows
the rejected world-body study without reusing its failed geometry. Production
Kit, the player world body and first-person arms remain unchanged.

The existing owner-cleared Meshy runtime is SHA-256
`1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713`:
26,945 vertices,25,968 triangles,24 joints and seven clips. Its existing licence
statement remains in ASSET_LICENSES.md; no generation, retargeting or purchase.

## Useful and unhelpful options

Locking every UV seam plus24-direction bind contour preserves26,608 rows and
leaves only53 triangles eligible, approximately0.1% theoretical reduction when
halving that set. This is not a useful candidate. A separate complete-byte-row
count finds **zero duplicates** among all26,945 position/normal/UV/tangent/joint/
weight tuples, so lossless duplicate-row compaction offers no vertex saving.
Position+UV-to-skin recovery has zero ambiguous groups. [Exact count](lossless-row-count.json).

One narrower trial is technically defensible:

- Retain exact Head/neck-influenced rows and all bindY≥1.55m, preserving head/hood
  geometry rather than relying only on an overall decimation target.
- Retain boots bindY≤0.45m, Hand-influenced contacts and original one-ring
  neighborhoods around source boundary/nonmanifold edges.
- Keep every source triangle incident to those protected rows. Preserve all
  rig/clip/acting data, materials and surviving attribute bytes.
- Propose a bounded-error reduction of remaining torso/limb triangles, then
  validate contour and deformation through actual native RT. Exact locking of
  every UV/contour row is not claimed.

That essential set protects11,428 rows/11,075 triangles, leaving14,893 triangles
and15,305 referenced rows eligible. Halving eligible triangles would total about
18,522, a **theoretical28.7% reduction**, not an optimizer result or timing forecast.
The current `-se0.005` proposal is **0.5% relative error**, not an absolute5mm
guarantee. No more permissive/aggressive simplification is proposed.

The inherited source has one welded component, Euler-6,17 boundary edges and16
nonmanifold edges (11 triple/5 quadruple); no opposed, zero-area or zero-UV
triangles. Preserve that baseline explicitly: no new normal/sliver/UV/topology
defects, no new boundary/component/grounding/contact issue. No reduction is
accepted merely because it imports or has fewer triangles. The world's two
nonmanifold-edge failure cannot be repeated silently here.

## Actual original view and remaining acceptance

The fresh [native RT reunion control](original-reunion-rt.png) uses the original
Kit in an isolated Windows package: RTX5050, RayTracingPipeline,
Diagnostic/High,960×540 internal/output,100% scale. Existing chapter207 staging
and presentation gates pass, Kit is grounded with Keeper dissolve complete,
and the process exits zero in11.53s. Copied settings remain byte-identical;
capture mode creates no campaign session or gameplay input. Null foreground
readbacks remained null, not observed owner focus. No phone activity occurred.
[Receipt](receipt.json) binds the image, executable, shader bank and source.

The torso silhouette is visible, but the hood shadows obscure fine face detail.
This **one original control is insufficient** to accept conversational face or
front/side silhouettes. A viable candidate needs matched actual front, side and
conversation views, unchanged acting/contacts and moving poses; use a provisional
one-output-pixel contour tolerance, verified at stated distances/resolutions.
Do not brighten the candidate or alter exposure to hide differences. The same
player world/viewmodel must be used in both cohorts.

Next executable slice is one Kit trial with these explicit protections and
static rejection gates, followed by actual RT inspection and a separate serial
CPU/frame comparison only if its geometry passes. Keep originals available.
Any phone gain needs a new owner-offered, verified connection and comparable
conditions. No Kit performance gain, completed LOD system or release claim is made.
