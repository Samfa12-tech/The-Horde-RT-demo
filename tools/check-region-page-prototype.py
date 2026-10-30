"""Check a private shader matrix against unchanged production frozen budgets."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("matrix", type=Path)
    parser.add_argument("receipt", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    budget_path = root / "tools/raygen-variant-budgets.json"
    budgets = json.loads(budget_path.read_text())
    manifest = {r["name"]: r for r in json.loads((root / "tools/raygen-variants.json").read_text())["variants"]}
    stats = {}
    for path in args.matrix.rglob("raygen-stats.json"):
        row = json.loads(path.read_text(encoding="utf-8-sig"))
        if row["key"] in stats:
            raise RuntimeError("Duplicate variant evidence")
        stats[row["key"]] = (row, path.parent)
    if set(stats) != set(manifest) or len(stats) != 8:
        raise RuntimeError("All eight exact variant identities required")
    rows = []
    for budget in budgets["budgets"]:
        key = budget["key"]
        row, directory = stats[key]
        failures = []
        for metric in budgets["metrics"]:
            if type(row[metric]) is not int or row[metric] > budget["max"][metric]:
                failures.append(f"{metric}: {row[metric]} > {budget['max'][metric]}")
        for field, expected in budget["exact"].items():
            # shippingAllowed is in the source manifest, not compiler stats.
            actual = manifest[key][field] if field == "shippingAllowed" else row[field]
            if type(actual) is not type(expected) or actual != expected:
                failures.append(f"exact invariant {field}")
        assemblies = list(directory.glob("*.spvasm"))
        if len(assemblies) != 1:
            raise RuntimeError("Exactly one disassembly required per variant")
        assembly = assemblies[0].read_text()
        for required in ("OpCapability PhysicalStorageBufferAddresses", "OpCapability RayQueryKHR", "OpImageWrite"):
            if required not in assembly:
                failures.append(f"missing {required}")
        if "OpCapability Int64" in assembly:
            failures.append("unexpected shaderInt64 requirement")
        rows.append({"key": key, "bytes": row["bytes"], "instructions": row["instructions"],
                     "functions": row["functions"], "rayQueryInitializations": row["rayQueryInitializations"],
                     "failures": failures, "spirvSha256": row["compiledSpirvSha256"]})
    receipt = {"budgetSha256": hashlib.sha256(budget_path.read_bytes()).hexdigest(),
               "passed": not any(r["failures"] for r in rows), "variants": rows,
               "scope": "Private raygen compile/validator/cost feasibility only; no GPU presentation, QueryCompute or phone pass"}
    args.receipt.write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))
    if not receipt["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
