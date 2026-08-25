# Static glTF contract fixtures

The manifests are hand-authored schema-1 inputs. `valid-multi.glb` and its
header/chunk negatives are produced deterministically by
`generate-fixtures.ps1`; the C++ tests also construct narrowly varied GLBs in a
temporary directory so every parser rejection is exercised against real GLB
bytes instead of source-text inspection or mocks.
