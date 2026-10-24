# Fuller crown r1 — editable source sample, not adopted

Owner-authorized bounded study of the existing original pine/alder. Review the small sample before any broader replacement. No production tree, placement, collision, root/trunk, shader, material-family or normal shared64 setting changes.

The matched source previews show, **left to right in the front view: pine control, pine study, alder control, alder study**. The camera changes side for the back view. These are actual editable Blender meshes rendered with Cycles in a neutral studio, **not Horde RT views or a forest-depth acceptance**.

- [Front comparison](previews/crown-control-study-front.png)
- [Back comparison](previews/crown-control-study-back.png)
- Editable derivatives: `source/pine-fuller-crown-r1.blend`, `source/alder-fuller-crown-r1.blend`.
- Authoring/measurement: `study.py`, [exact input/output and geometry hashes](study.json).

## What changed and what did not

Each existing closed foliage lens expands about its original base vertex; the original base remains fixed. Pine width/length scales are 1.85/1.35, alder 1.65/1.25. The underlying original branch/trunk/root and moss mesh coordinates, indices and world transforms remain exact, verified before/after. There are no added branches, leaves, billboards, alpha cards, textures or new instances. Existing source provenance and owner rights from `forest-tree-pair-v1/README.txt` apply; no third-party or paid assets were used.

| Source sample | Triangles | Source vertices | Mesh/material families | Fixed leaf-base anchors |
| --- | ---: | ---: | ---: | ---: |
| Pine | 5,000, unchanged | 3,362, unchanged | 3/3 | 366 |
| Alder | 5,102, unchanged | 3,371, unchanged | 3/3 | 330 |

The foliage bounds grow modestly; trunk/root contact remains exact. Same counts do **not** prove equal BLAS memory or traversal/shadow cost: coverage, overlap and bounds change. No game CPU/GPU/memory gain, physical-phone result, game image or normal adoption is claimed.

## Visual verdict

Both source views were inspected. Larger foliage gives more weight to existing boughs, but the branches remain separated by large windows. The pine still reads as a sparse, tiered tree and alder as an open, young forked tree. This **does not deliver the surrounding dense woodland target** and is not recommended for broad rollout. Leaf enlargement alone is insufficient. Preserve this sample and the originals; the next worthwhile art question is a bounded upper-crown branch/foliage distribution study that keeps the exact trunk/root contact and placement, or an explicitly chosen terrain-enclosure composition. That is a further sample to coordinate with the parent, not an excuse to add a scatter quota or claim the forest finished.

## Export blocker and validation boundary

The pine `.blend` saved successfully. Blender's glTF exporter then failed to write its temporary image at the exact [denial target](export-blocker.json), outside the workspace, and its cleanup raised OS WinError5. No GLB was produced. The exporter and denied temp path were not retried, redirected, cleaned or permission-changed. Its tangent warning also remains in the raw log; no source topology change was made to suppress it.

Independent **source-only** authoring of alder and direct `.blend` preview rendering completed afterward without the exporter. The script deliberately has no export call; it loads editable source objects for the preview. Both `.blend` files can be reopened for further authoring. Runtime export/import, tangents, the actual F01/F04 views, secondary visibility, combined-scene memory and paired RT cost remain unverified/blocked; no runtime candidate is staged. Failed exporter log and first script remain under `reports/player-run-retarget-20261024/`. Do not retry the denied export action without new relevant authorization.

No phone, foreground activation, owner save/settings changes, private Easy Tree adoption, merge or release. Existing originals and all 151 parked owner files remain preserved. The complete flask policy is separately awaiting owner approval; smoke remains a required measured implementation, while new outdoor mist/shafts/fireflies/visual wind are explicitly deferred for initial 1.7.

Reopen validation passes for both derivatives: original topology/material assignments, UVs, fixed foliage bases, exact wood/moss and every referenced packed image remain. The first all-global-image comparison failed because ordinary Blender save omitted an unused pine image from the alder file; the retained diagnostic establishes that it is unreferenced, with no changed referenced texture. No production test was altered. `validation.json` and `validate.py` record this corrected measurement scope.

The existing source/runtime/package asset-policy CTest passes1/1 with the new source directory present. Source files stay excluded from runtime packaging; this does not validate a runtime crown export. Original runtime tree hashes and the151 owner-file hashes remain unchanged.
