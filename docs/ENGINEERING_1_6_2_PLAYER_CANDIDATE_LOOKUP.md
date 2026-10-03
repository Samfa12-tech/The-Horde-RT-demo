# Bounded primary player candidate lookup experiment

This is a correctness-preserving optimization candidate, with no measured saving
claimed yet. Released a397 and pre-experiment c299 both perform the same dynamic
instance-role lookup for every primary ray-query candidate. The older167ce8
historical benchmark predates the accepted dedicated modelled viewmodel; its
35% whole-frame/40% GPU comparison cannot isolate a1.6.2 regression or this lookup.
The released-source same-player Shipping comparison is collected separately.

The shader now checks the generated dedicated WorldBody instance index before
its existing role lookup. Primary head/near-face filtering and explicit body
remainder predicates remain intact; dedicated primary viewmodel and complete
secondary/reflection/shadow body visibility remain unchanged. No masks, geometry,
materials, lights, quality, scale or gameplay cadence change.

Generic asset registration legitimately allows player-role aliases. Its loader
is unchanged. Before GPU metadata upload, Showcase and Preview each perform
one scene admission scan over all22 metadata records: player roles may appear
only at their generated dedicated indices4/20. Absent roles remain valid for
Preview. Misplaced aliases are rejected with diagnostics, including unpopulated
records; there is no per-frame admission scan. Both CPU and GLSL indices derive
from the existing ABI definition. Its943cb6bd hash, layouts and capacities remain
unchanged.

Actual SDK disassembly of published ShippingMobile Pipeline and Compute modules
proves the slot-equality control-flow edge protects the candidate role
AccessChain and Load. Removing that decisive edge makes both instructions
unreachable in the reconstructed graph. A preserved unguarded module fails this
specific check. Both retain23 hardware ray-query initialization sites, actual AS
types/proceed/confirm, one inlined function/no calls, no binding22 and zero
atomics. This establishes compiled behavior, not hardware timing.

All eight Pipeline and eight Compute modules, source-bound catalogs/adapters and
compatibility includes are regenerated. Independent word extraction verifies
module hashes, dependencies and raw compatibility-byte equality. Frozen Pipeline
ceilings change only to exact measured values: each Generic module adds56bytes,
14words and3instructions; each Opaque module adds100bytes,25words,7instructions,
2branches and1selection merge. Other metrics and exact invariants remain
unchanged, with no extra headroom. The initial strict old-ceiling rejection is
preserved; the guard was not bypassed. Existing source fixtures refresh only
independently derived hash literals.

Windows app and ABI targets build in both configurations. Debug executable
SHA256 `35d492724d44d6927de52b5b40c7374ffc5e8235fb6a4da6b82d12964ae29bba`;
Release `52d6954fcf5b566ff55b455eb1e3e2e8b026326f00008ec4d0a9aef0facb3dca`.
Six affected Debug ABI/generator/manifest/artifact/Compute/policy checks pass
219.92s; Release ABI passes0.94s. Earlier configuration/command failures remain
preserved: missing explicit absolute freeze paths, old exact byte ceilings,
missed Pipeline adapter regeneration and nonexistent executable-name targets.
None is promoted to a successful application build.

Remaining gates are refreshed Android admission, actual native image equivalence
and cold matched same-artifact phone timing. Adopt or disposition this experiment
from those results. Existing3,287-pose kinematic proof is unaffected by this
metadata/lookup change; no new grip/headroom result is inferred from shader tests.
