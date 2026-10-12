"""Recover the supplied package without rewriting a master or existing revision."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--source-package", type=Path, required=True)
p.add_argument("--tree-package", type=Path, required=True)
p.add_argument("--source-destination", type=Path, required=True)
p.add_argument("--runtime-destination", type=Path, required=True)
p.add_argument("--repository-root", type=Path, default=Path.cwd())
a = p.parse_args()
if a.source_destination.exists() or a.runtime_destination.exists():
    raise RuntimeError("Preserve existing source/runtime revisions.")
manifest = json.loads((a.source_package / "sha256-manifest.json").read_text())
for name, entry in manifest.items():
    source = (a.source_package / name).resolve()
    if not source.is_relative_to(a.source_package.resolve()):
        raise ValueError("Source manifest escapes its package.")
    data = source.read_bytes()
    if len(data) != entry["bytes"] or hashlib.sha256(data).hexdigest() != entry["sha256"]:
        raise ValueError(f"Original payload differs: {name}")
alder = (a.source_package / "textures/horde-alder-original-basecolor-512.png").read_bytes()
matches = [q for q in a.tree_package.rglob("*.png") if q.read_bytes() == alder]
if not matches:
    raise ValueError("Fern albedo must match the supplied admitted tree-pair map.")
m = json.loads((a.repository_root / "assets/models/world/runtime/forest-tree-pair-v1/asset.manifest.json").read_text())
m["asset"] = "original-forest-understory"
m["budgets"] = dict(maxVertices=4000, maxIndices=4000, maxPrimitives=1,
                    maxMaterials=1, maxTextureLayersPerKind=1)
m["lods"] = [dict(name="lod0", maxTriangles=1200)]
a.source_destination.mkdir(parents=True)
shutil.copytree(a.source_package, a.source_destination / "original-package")
a.runtime_destination.mkdir(parents=True)
files = []
for kind in ("open", "small"):
    dst = a.runtime_destination / f"horde-fern-{kind}-clump.glb"
    shutil.copyfile(a.source_package / f"assets/bh_fern_{kind}_clump.glb", dst)
    files.append(dict(path=dst.relative_to(a.repository_root / "assets").as_posix(),
                      bytes=dst.stat().st_size, sha256=hashlib.sha256(dst.read_bytes()).hexdigest(),
                      derivation="byte-identical original"))
mp = a.runtime_destination / "asset.manifest.json"
mp.write_text(json.dumps(m, indent=2) + "\n", newline="\n")
files.append(dict(path=mp.relative_to(a.repository_root / "assets").as_posix(),
                  bytes=mp.stat().st_size, sha256=hashlib.sha256(mp.read_bytes()).hexdigest(),
                  derivation="bounded native intake manifest"))
receipt = dict(schema=1, originalPayloadsVerified=len(manifest), runtime=files,
               albedoSha256=hashlib.sha256(alder).hexdigest(),
               exactTreeMapMatches=[q.relative_to(a.tree_package).as_posix() for q in matches],
               features="closed opaque foliage; no normals/alpha/transparency/light or new texture family",
               material="existing ForestAlder with exact shared source albedo",
               sourceOnly="All seven original masters, maps, scripts and provenance preserved; only two ferns admitted.")
(a.source_destination / "admission-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", newline="\n")
print(f"Verified {len(manifest)} original payloads; copied two byte-identical native candidates.")
