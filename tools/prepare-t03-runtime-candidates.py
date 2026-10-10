#!/usr/bin/env python3
"""Copy the selected original T03 GLBs byte-for-byte into the runtime candidate tree."""

from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "assets/source/beyond_the_tomb/funerary/t03-v01"
RUNTIME_ROOT = ROOT / "assets/models/world/runtime/prepared-funerary-v01/t03-native-import-candidates"
SELECTED = (
    "t03_displaced_lid",
    "t03_offering_bowl",
    "t03_urn_broken_base",
    "t03_urn_rim_shard",
    "t03_candle_stub_1",
    "t03_candle_stub_2",
    "t03_candle_stub_3",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    manifest_path = SOURCE_ROOT / "asset-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    records = {record["id"]: record for record in manifest["assets"]}
    if set(SELECTED) - records.keys():
        raise SystemExit("Selected T03 ids are missing from the recovered source manifest.")
    RUNTIME_ROOT.mkdir(parents=True, exist_ok=True)
    recovered = []
    for asset_id in SELECTED:
        record = records[asset_id]
        source = SOURCE_ROOT / record["model"]
        destination = RUNTIME_ROOT / source.name
        if not source.is_file():
            raise SystemExit(f"Missing recovered T03 source payload: {source}")
        source_hash = sha256(source)
        if source_hash != record["glb_sha256"]:
            raise SystemExit(f"Source manifest hash mismatch for {source.name}")
        if destination.exists() and sha256(destination) != source_hash:
            raise SystemExit(f"Refusing to replace a different runtime candidate: {destination}")
        if not destination.exists():
            shutil.copyfile(source, destination)
        runtime_hash = sha256(destination)
        if runtime_hash != source_hash:
            raise SystemExit(f"Runtime copy hash mismatch for {destination.name}")
        recovered.append(
            {
                "id": asset_id,
                "source": source.relative_to(ROOT).as_posix(),
                "sourceBytes": source.stat().st_size,
                "sourceSha256": source_hash,
                "runtime": destination.relative_to(ROOT).as_posix(),
                "runtimeBytes": destination.stat().st_size,
                "runtimeSha256": runtime_hash,
                "triangles": record["triangles"],
                "geometryChanged": False,
                "materialsOrUvsChanged": False,
            }
        )
    receipt = {
        "status": "runtime candidate; not admitted",
        "sourcePackage": SOURCE_ROOT.relative_to(ROOT).as_posix(),
        "sourceManifestSha256": sha256(manifest_path),
        "sourceReadmeSha256": sha256(SOURCE_ROOT / "README.md"),
        "sourceChecksumListSha256": sha256(SOURCE_ROOT / "SHA256SUMS"),
        "sourceAuthority": "Recovered Horde-owned A17/T03 source package; per-asset GLB hashes match its asset-manifest.json and SHA256SUMS.",
        "rights": "Original Blender-authored project geometry and PBR parameters; the source README states no third-party model/texture bytes or external stock-asset license. No external acquisition or generation was performed for this runtime copy.",
        "recipe": "Byte-exact file copy from each manifest-selected source GLB into the runtime candidate directory; no geometry, indices, materials, UVs, or textures changed.",
        "assets": recovered,
    }
    output = RUNTIME_ROOT / "derivation-receipt.json"
    output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(f"Prepared {len(recovered)} byte-exact T03 runtime candidates.")
    for item in recovered:
        print(f"{item['id']}: {item['runtimeBytes']} bytes, {item['triangles']} triangles, sha256={item['runtimeSha256']}")


if __name__ == "__main__":
    main()
