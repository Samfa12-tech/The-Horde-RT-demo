# Bellwether tavern detail candidates

The B01 exports use metres and identity transforms in the registered town position. All three share the same 494-triangle architecture. It includes the roof, chimney, foundation, approach ramp, fixed opening frames, room floor, hearth and candle. The [authoring receipt](authoring-receipt.json) pins the tavern Blender master, the window glass and hinged door shared by the three levels, and each candidate.

| Furnishings | Chair and table geometry | Triangles |
| --- | --- | ---: |
| `lod0` | Original timber parts and bevels | 1,012 |
| `lod1` | Original part bounds with square edges | 276 |
| `lod2` | Merged seat and top boards, full-height posts and legs, simplified framing | 144 |

The native import check loads all four GLBs with their exact manifest budgets. Every architecture and near-furniture triangle matches an oriented triangle in the current town export on a 0.1 mm coordinate grid. The south doorway and window apertures stay open in the architecture. All three furniture levels preserve the same bounds, timber finish, seat and tabletop heights. Khronos glTF Validator reported zero warnings and errors on all four GLBs. Two Blender 5.2 exports produced byte-identical GLBs, manifests and receipts.

## Native comparison

Windows Debug can load a fixed tier into the chapter's existing static RT geometry before it builds the BLAS. It replaces only the registered furniture triangles under the original timber material. The authored collision is built from the original town geometry, and the window, door and light configuration are shared across tiers.

From the repository root, use a different output directory for each take. Omit the detail flag for the original mesh, select `lod1` or `lod2` for a comparison, and add `--require-rayquery-compute` to repeat the take on the other hardware RT backend.

```powershell
$hordeDebug = '.\build\presets\windows-x64-debug\Debug\HordeLanternRT.exe'
& $hordeDebug --development-rescue-journey --capture-hidden --capture-showcase .\build\bellwether-approach-lod2 --development-checkpoint bellwether-south-approach --bellwether-furniture-detail lod2
```

The window view uses `bellwether-tavern-window`. The staffed view uses `bellwether-staffed-room` with the corresponding local `--chapter-resident-fixture`. The comparison manifest records the selected lower tier and its triangle count. In the staffed room the reported static vertex and index payloads together fell by 135,040 bytes at `lod1` and 153,520 bytes at `lod2` on both backends.

Twelve presented Windows frames at 960 x 540 on an RTX 5050 Laptop GPU used matched cameras, shader bundles and presentation settings within each pair. In the staffed room, 386 pixels changed at lod1 on each backend and 716 to 717 pixels changed at lod2. At the south approach, the far tier changed 132 pixels on each backend, confined to the visible room. The RayTracingPipeline window pair with Glass On was pixel-identical. These are full-frame comparisons. Individual primary, shadow and reflection contributions still need dedicated validation.

## Automatic selection

Replace the fixed detail flag with `--bellwether-furniture-transitions` for a hidden Windows Debug capture. The loader separates the exact original furniture from the town mesh after it creates authored collision. Each tier has an immutable BLAS. Every frame publishes one tier through the chapter's spare instance and the original timber material, while the shared architecture, door, window and lights keep their owners.

Selection uses horizontal distance to the full room bounds. Near enters at 4 m and stays active through 6 m. Far enters beyond 20 m and stays active until 16 m. Mid fills the band in between. This protects the immediate doorway and both sides of the window, and gives a turning player a stable tier near each boundary. All three tiers stay resident for this experiment. The south-approach capture reported 58,800 additional vertex and index bytes and 34,816 additional BLAS bytes compared with the original merged geometry.

Eight Windows frames presented on the two RT backends. Both chose far at the church waymarker, mid at the south approach, and near in the staffed room and at the window. Compared with fixed near, both staffed-room images and the matched pipeline window image with Glass On were pixel-identical. The mid tier changed 52 pixels at the approach on each backend, all within the visible room. The native host test exercised both hysteresis bands on entry, retreat and repeat approach.

For a moving check, add `--bellwether-furniture-route` and use `bellwether-repeat-visit` with its local tavern keeper fixture. The shared simulation owns the road, open doorway, exchange checkpoint and two quiet returns. The Windows runner presents one frame every 12 simulation ticks and on each tier or room-boundary change. It checks the recorded TLAS furniture address and the active door and pane, then joins milestone readbacks to the exact completed presentation. `furniture-route.csv` records the accepted frame sequence and `furniture-route.json` is written only when the full route passes on a single TLAS.

Both Windows RT backends presented 1,033 sampled frames across three entries and two quiet returns. The sampled route logs matched byte for byte, with one TLAS per run and 13 completed image readbacks per backend. The tier sequence was far, mid, near, mid, near, mid, near. The authored door and pane stayed active throughout. The route used the local tavern keeper fixture and hidden 960 x 540 presentation.

## Ray contribution diagnostic

The private Windows Debug witness counts pixels whose existing hardware RT queries hit the split furniture instance. It records the committed primary hit, shadow blockers, reflected hits and transmitted hits in the low four red bits of the presented image. The bits are 1, 2, 4 and 8 respectively. A pixel can carry more than one bit. The shaders keep the normal color in the upper red nibble and the green and blue channels to support a matched ordinary capture.

From the repository root, prepare a fresh private shader tree, then generate its catalogs with the normal shader tools.

```powershell
python tools/prototype-bellwether-ray-witness.py build/bellwether-ray-witness
$witnessRoot = (Resolve-Path build/bellwether-ray-witness).Path
& "$witnessRoot/tools/compile-raygen.ps1" -Freeze -ArtifactDirectory "$witnessRoot/src/vulkan/raytracing/variants" -CatalogPath "$witnessRoot/tools/raygen-variant-catalog.json" -BudgetPath "$witnessRoot/tools/raygen-variant-budgets.json"
& "$witnessRoot/tools/GenerateRtPipelineVariantCatalog.ps1" -Write
& "$witnessRoot/tools/GenerateRayQueryComputeVariants.ps1" -Write
```

Use a separate Windows build directory with `HORDE_RT_BELLWETHER_RAY_WITNESS_ROOT` set to the absolute private directory, and build Debug. Launch the same hidden automatic-detail checkpoint as above. The capture manifest records the private encoding and the exact selected shader hashes. Decode each RGB PNG with `red & 15`.

Both backends reported the same counts in all five matched 960 x 540 views.

| View | Primary | Shadow | Reflection | Transmission |
| --- | ---: | ---: | ---: | ---: |
| Church waymarker | 0 | 0 | 0 | 0 |
| South approach | 216 | 910 | 38 | 0 |
| Staffed room | 1,786 | 8,006 | 700 | 0 |
| Window with Glass On | 0 | 1 | 0 | 0 |
| Hearth | 5,408 | 27,719 | 2,483 | 0 |

In the staffed room, 6,524 shadow pixels and 423 reflection pixels had no primary furniture hit. The window gave a shadow hit even though the furniture lay entirely outside direct visibility in that frame. The matched ordinary captures retained identical green and blue channels. These sampled views establish why a furniture tier must remain available for secondary and shadow rays when its primary hits disappear. Safe resource retirement and phone budgets remain acceptance gates.

The [Blender recipe](../../../../../tools/author-bellwether-tavern-lods.py) takes the repository root and a new output directory. The existing `horde_rt_bellwether_shell_tests` executable accepts that output directory as an optional argument to run the candidate checks alongside its ordinary town tests.
