# Reuse one existing provider compile

Base: `719b86aabd52cc024d74e92629b5de84c8d47136`. This is a test-build
change in `CMakeLists.txt`, with no runtime, shader, asset or workflow change.
The provider and bundle-contract tests now link the existing Shipping/Mobile
`horde_rtpv_0_0_obj` instead of each compiling that same provider again. The
other three policies retain distinct compiler-produced objects, and all four
native fixtures and object-containment wrappers remain.

## Equivalence and scheduling

- Generated MSVC compile settings match for the old consumers and shared object
  in all four configurations. Actual Release compiler commands match apart from
  object/PDB output paths. There are six provider compiles before, four after.
- Both consumers have exactly one object input and a project dependency on its
  producer. No additional CTest process, parallel test execution or shared
  scratch ownership is introduced.
- All 185 configured CTest names, commands and properties match after normalizing
  the two build-output paths. The actual Windows selector still selects 149,
  with the same 21 portable and 15 tool exclusions. Per-test deadlines match.
- All 109 non-metadata COFF sections match across the two original consumer
  objects and control/candidate shared objects. An initial ad-hoc whole-section
  equality assertion failed: `.debug$S` records their different absolute object
  paths, and only its corresponding `.chks64` entry differs. That failure is
  retained; whole-object hashes are deliberately not labelled identical. No
  production acceptance predicate was changed.

## Focused local observations

MSVC 19.44.35207, Visual Studio 2022 generator, x64 Release, Vulkan targets OFF,
`--parallel 2`. Separate clean build-output directories in the existing worktree
used the same six explicit targets: the two consumers and four fixtures. This
does not duplicate the repository and is not the whole hosted `ALL_BUILD` workload.

| Observation | Control | Object reuse |
| --- | ---: | ---: |
| Configure | 18.80 s | 19.21 s |
| Six-target clean build | 25.38 s | 23.99 s |
| Ten existing provider/fixture/evidence tests | 122.62 s, 10/10 pass | 131.77 s, 10/10 pass |

This is one local pair, not a statistical CI speed claim. The 1.38-second build
difference cannot establish robust headroom for the 15-minute MSVC job. Wrapper
test time increased; the parsing/scanning implementation was unchanged in this
comparison. No phone or game-performance claim follows.

The existing policy-fence test also checks fresh configuration, resolved fixture
runners, catalog rejection controls and unchanged 300-second policy deadlines.
Its result is recorded in [provider-reuse-receipt.json](provider-reuse-receipt.json).

Raw local logs, generated project/CTest snapshots and the failed initial COFF
assertion remain under `reports/msvc-ci-dedup-20261023/`. The compact receipt lists
their hashes. No generated projects, compiler objects or executable payloads are
committed. The separately proposed MSVC-only 20-minute cap remains unapplied.
