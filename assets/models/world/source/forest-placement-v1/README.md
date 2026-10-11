# Authored woodland placement derivatives

These placement plans are Horde scene-authoring data, separate from the preserved
48-file original tree package in `../forest-tree-pair-v1/`. They do not alter its
masters, licence or closed SHA-256 roster.

`woodland-placements-v2.json` preserves the earlier centre-grounded placement
input. `woodland-placements-v3.json` is the root-footprint correction generated
by `tools/fit-forest-root-placements.py`. Both files were moved here byte-for-byte
after CI correctly rejected adding scene derivatives to the original package.
`tools/build-forest-placement-contract.py` consumes v3. Both tools have read-only
freshness checks; native geometry and traversal tests remain authoritative.
