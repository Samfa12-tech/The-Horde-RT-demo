# Fitted niche wall joins — streaming prerequisite

This implementation preserves the three native niche modules and their contents,
fits both real wall planes under their existing stone frames, and follows the
arched head with seven wall panels. It fixes source-triangle escape segments;
it does **not** enable static-region eviction or certify the entire tomb shell.
Normal static-region residency saving remains **zero**.

## Reproduction and correction

Against the original 426,818 native plus 2,012 immutable procedural triangles,
these exact segments had no intersections:

| Segment | Start → end | Final first hit |
|---|---|---|
| Entry lower-Z edge | (-1.84,0,-1.0479) → (-2.17,0,-1.0479) | Visible wall X=-1.850000024 |
| Entry upper-Z edge | (-1.84,0,-.3521) → (-2.17,0,-.3521) | Visible wall X=-1.850000024 |
| Above the arch | (-1.7,.19,-2.6) → (-2.25,.5475,-2.6) | Wall at Y=.287499995 |

The declared cuts used module outer bounds. The rectangular cut exceeded its
native edge; the arched rectangular cut allowed rising rays above the crown.
New cuts are fitted to actual native triangle sections. The arch bottom follows
the wide lower mass rather than the narrower shelf, retaining the corner join.
The native backs remain at their original depths: the entry control ray still
hits X=-2.076659918. No asset/master, collision, texture, shader, mask, material
family or quality limit changed. The full native geometry dump remains SHA-256
`81047d12abe713ff3566e0434d61e1571431612fda159c2c3db2c13bc4436edf`.

The actual builder adds 24 procedural triangles: 2,188→2,212 total,
2,012→2,036 immutable. That adds 48 vertices, 72 indices and 24 surface records
(960 encoded bytes before allocator/AS granularity); it adds no static owner,
material, texture layer or descriptor. Actual GPU allocation remains separate.

## Inspected native frames and their limits

Matched captures use 960×540, 100% scale, High optical profile, Higher shadows,
12 frozen settling frames and the NVIDIA RTX 5050 Laptop GPU. Exact executable,
source/profile, shader and capture identities are in the receipts/archive.
Both hardware backends used identical shader identities before/after.

These close niche views remain **rejected** by the strict primary-arm gate in
both baseline and candidate. Actual uploaded baseline arms projected outside
the inspection frustum. Changing pitch/prop fixtures did not close that gate;
an attempted parry fixture could not stage and was not retained as a checkpoint.
Failures and uploaded meshes are preserved. No ownership assertion was waived,
and these environmental frames are **not** full visual/player acceptance.

The frames were opened and compared: the original empty region over the arch is
now stone; the native arched void, back, skull, candles and frame silhouettes
remain. Pipeline comparisons changed 3,703/4,468/162 pixels for rect/arch/east;
Query comparisons changed 3,707/4,467/160. The established side-wall carry view
passes on both backends and is decoded-pixel-identical to the prior cfd381a1
captures, including that view's surface shadows and equipment.

| View | Before | After |
|---|---|---|
| Entry rectangular | [Pipeline](gallery/pipeline-rect-before.png) / [Query](gallery/query-rect-before.png) | [Pipeline](gallery/pipeline-rect-after.png) / [Query](gallery/query-rect-after.png) |
| Entry arched | [Pipeline](gallery/pipeline-arch-before.png) / [Query](gallery/query-arch-before.png) | [Pipeline](gallery/pipeline-arch-after.png) / [Query](gallery/query-arch-after.png) |
| Outer passage | [Pipeline](gallery/pipeline-east-before.png) / [Query](gallery/query-east-before.png) | [Pipeline](gallery/pipeline-east-after.png) / [Query](gallery/query-east-after.png) |

## Checks and next gate

Native tests reproduce the original three misses, require their real wall hits,
retain native back controls and check 148 actual frame-perimeter rays on both
wall planes. Debug/Release affected five-test selections pass; final registry
and authored-route checks pass. Fresh Android Java: 286 tests, no failures,
errors or skips; four-ABI Debug build/lint and strict ASTC/closed-roster package
checks pass. Final artifact hashes are in `artifact-receipt.json`. These are
intermediate uninstalled artifacts, not a completed chapter/phone candidate.
Exact-commit CI is recorded in the PR after pushing; no CI pass is inferred here.

Full section analysis found 25–86 nm radial seams between native voussoirs.
Convex hulls concealed those gaps; their apparent whole-strip pass was rejected.
The rear contains the return/arch sections, but an all-direction volumetric
witness is still unproved. For eviction, treat the arch as a potential opening
until a conservative contributor certificate proves otherwise. Do not snap or
regenerate the accepted master, silently fill geometric tolerance, or use these
sample rays as an all-ray proof.

Next: continue contributor-safe normal two-way static preparation/publication,
completed-work retirement and reload, retaining coarse world/landmarks and any
uncertain secondary/light contributors. This checkpoint closes only the three
reproduced wall-cut escape regressions. Controlled optimized Release pacing and
physical phone/owner acceptance remain open. The owner's live desktop CPU/GPU
readout below 60% is contextual evidence, not attribution of the earlier stalls;
no new foreground performance run or phone operation was made here.
