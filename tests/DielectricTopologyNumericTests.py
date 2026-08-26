"""Reference checks for cgltf float expressions and exact weld-cell limits."""

from __future__ import annotations

import importlib.util
import math
import pathlib
import struct


ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXTURE_ROOT = ROOT / "tests/fixtures/dielectric-topology"
VALIDATOR_PATH = ROOT / "tools/validate-dielectric-topology.py"

spec = importlib.util.spec_from_file_location("dielectric_topology_validator", VALIDATOR_PATH)
validator = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(validator)


for line in (FIXTURE_ROOT / "weld-cell-domain-cases.txt").read_text(
        encoding="utf-8").splitlines():
    if not line or line.startswith("#"):
        continue
    name, hexadecimal, accepted_text = line.split()
    scaled = float.fromhex(hexadecimal)
    accepted = accepted_text == "true"
    try:
        cell = validator.weld_cell((scaled, 0.0, 0.0), (0.0, 0.0, 0.0), 1.0)
        actual = True
    except validator.ValidationError:
        cell = None
        actual = False
    assert actual == accepted, (
        f"{name}: exact cell coordinate {hexadecimal} acceptance was {actual}, "
        f"expected {accepted}")
    if accepted:
        assert cell[0] == math.floor(scaled), (
            f"{name}: accepted cell did not preserve exact floor semantics")


document, _ = validator.read_glb(
    FIXTURE_ROOT / "large-trs-matrix-seam-dielectric-lod0.runtime.glb")
world = validator.node_world_matrices(document)
trs_bits = struct.pack("<16f", *world[1])
matrix_bits = struct.pack("<16f", *world[3])
assert trs_bits == matrix_bits, (
    "cgltf-equivalent parent/child TRS and matrix paths must produce bit-identical "
    "world transforms")

print("Dielectric topology exact numeric reference contracts passed")
