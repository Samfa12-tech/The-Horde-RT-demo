"""Source provenance checks for the development rescue blockout generator."""
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import shutil
import time


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    tool_path = repo / "tools/author-rescue-blockout.py"
    spec = spec_from_file_location("author_rescue_blockout", tool_path)
    require(spec is not None and spec.loader is not None, "blockout authoring module is loadable")
    tool = module_from_spec(spec)
    spec.loader.exec_module(tool)

    materials = tool.validate_material_sources(repo)
    require([(item["name"], item["surfaceCode"], item["worldTextureLayer"])
             for item in materials] == [("DryStone", 0, 0), ("MossyStone", 2, 2)],
            "admission binds the existing dry/mossy world material codes and layers")

    root = repo / f".tmp-rescue-source-admission-{time.time_ns()}"
    root.mkdir()
    try:
        try:
            tool.validate_material_sources(root)
        except ValueError as error:
            require("missing source map" in str(error), "missing source map fails with an explicit diagnostic")
        else:
            raise AssertionError("missing source path was admitted")

        name, _, _, source_name, _ = tool.MATERIAL_SOURCE_SPECS[0]
        tampered = root / "assets/textures/polyhaven/mobile_1k" / source_name / "diff.jpg"
        tampered.parent.mkdir(parents=True)
        tampered.write_bytes(b"not the admitted source bytes")
        try:
            tool.validate_material_sources(root)
        except ValueError as error:
            require("source map hash mismatch" in str(error) and name in str(error),
                    "changed source map fails with its material and hash diagnostic")
        else:
            raise AssertionError("changed source map was admitted")
    finally:
        if root.parent.resolve() == repo.resolve() and root.name.startswith(".tmp-rescue-source-admission-"):
            shutil.rmtree(root)
    print("Rescue blockout source admission passed: exact existing hashes, missing path, and changed bytes.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
