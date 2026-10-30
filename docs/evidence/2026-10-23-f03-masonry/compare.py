"""Read-only admission facts from the retained paired native RT manifests."""
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
rows = []
for view in ("forward", "reverse", "lookout"):
    control = json.loads((HERE / f"control-{view}-manifest.json").read_text(encoding="utf-8-sig"))
    candidate = json.loads((HERE / f"{view}-manifest.json").read_text(encoding="utf-8-sig"))
    a, b = control["captures"][0], candidate["captures"][0]
    for m, capture in ((control, a), (candidate, b)):
        assert m["complete"] and m["error"] is None and capture["honestlyPresentedRtFrame"]
        assert m["executionBackend"] == "RayTracingPipeline"
        assert m["timing"]["sampleCount"] == 12
    for key in ("device", "presentation", "selectedRtPipelineBundle"):
        assert control[key] == candidate[key], key
    for key in ("camera", "requestedCamera", "state", "rescueTraversal"):
        assert a[key] == b[key], key
    for key in ("instanceMasks", "primaryPixels", "verticalProofPoseWitness"):
        assert a["visibility"][key] == b["visibility"][key], key
    old, new = control["staticRtAsset"], candidate["staticRtAsset"]
    for key in ("materialBytes", "instanceMetadataBytes", "primitiveMetadataBytes", "textureBytes", "descriptorCount"):
        assert old[key] == new[key], key
    deltas = {k: new[k] - old[k] for k in ("vertexBytes", "indexBytes", "productionPropBlasBytes")}
    assert deltas == {"vertexBytes": 18432, "indexBytes": 1728, "productionPropBlasBytes": 7296}
    same_image = a["pngSha256"] == b["pngSha256"]
    assert same_image == (view == "lookout")
    old_time, new_time = control["timing"], candidate["timing"]
    rows.append({"view": view, "samplesEach": 12,
                 "controlFrameWallMedianMs": old_time["overallMedianMs"],
                 "candidateFrameWallMedianMs": new_time["overallMedianMs"],
                 "frameWallChangePercent": 100 * (new_time["overallMedianMs"] / old_time["overallMedianMs"] - 1),
                 "controlGpuCommandMeanMs": old_time["gpuRtCommandBuffer"]["averageMs"],
                 "candidateGpuCommandMeanMs": new_time["gpuRtCommandBuffer"]["averageMs"],
                 "samePng": same_image, "resourceDeltas": deltas})
print(json.dumps({"scope": "Cold 12-frame Diagnostic capture comparison, frozen scene. Not sustained performance, CPU utilisation, natural journey or Android acceptance.", "comparisons": rows}, indent=2))
