# High glass-edge contact: demonstrated discriminator, not a fix

Engineering renderer/modules/defaults remain unchanged. Isolated observer source
48702729ab7ccc0bddda91517da9b42bc6efa5e2, complete pushed evidence
741c755ebd15fc6ebb560a1f57cbb9e4bff96796 on codex/horde-rtx-corrections.
Exact Debug/High/Diagnostic executable SHA-256
9ed552b897ecfd719cebc223a1ea79f9244eb7071b9350b783f9c821769c05af.

Exactly one glass-edge-fresnel native capture per RTX5050 Laptop backend: exit0,
honest RT. Only one explicitly reserved RGBA8 payload row; no extra rays/samples,
buffers/ABI or budget changes. All nonpayload rows1-539 and41 diagnostic/reason
counters match completed clean controls exactly. Both watched RGBs reproduce:
pipeline22,17,17; compute2,2,3. Backend gate still FAILS max20/32 pixels over3,
unchanged max3/fraction0.001. Controls/full matrices were not repeated.

Primary9/10/107, direction, origin, t and geometric normal agree; actual reflection
direction, origin, hit0/496/2 and radiance agree exactly too. After four internal
reflections, visit5 has t0.3171285/world y=-0.95 on both paths. Pipeline selects
glass bottom9/8/107 (normal down); compute selects floor0/486/1 (normal up).
The selection of coincident glass-exit/opaque-floor surfaces is demonstrated;
the underlying compiler/driver tie mechanism and other failing pixels are not.

Compute absorbs transmission by existing certified reason1 recovery with an open
volume. Pipeline closes the volume, spawns below the same floor (y=-0.9500308),
then misses into sky. Neither is automatically the physical reference; forcing
the brighter baseline could preserve a floor leak. Counts remain566/568 certified
recoveries, TIR13842/13843. No diagnostic suppression or false physical pass.

Two changed modules compile/optimise/disassemble/validate;14 others retain exact
artifacts/provenance, not a new compilation claim. App build PASS, provider1/1
PASS1.91s, generated pipeline adapter PASS. Frozen cost admission remains failed;
no performance/Android/production promotion claim. Negative wrapper repairs and
negative paired-image gate are retained, not hidden. Isolated cache restored to
Mobile/Shipping without rebuilding unchanged artifacts; normal configuration never changed.

Durable isolated [full record](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/741c755ebd15fc6ebb560a1f57cbb9e4bff96796/docs/evidence/2026-10-02-backend-pixel-witness/high-edge-observer.md),
[native fields](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/741c755ebd15fc6ebb560a1f57cbb9e4bff96796/docs/evidence/2026-10-02-backend-pixel-witness/high-edge/decoded.json),
[comparison](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/741c755ebd15fc6ebb560a1f57cbb9e4bff96796/docs/evidence/2026-10-02-backend-pixel-witness/high-edge/comparison.json).
Both PNGs/manifests/build/process logs are retained there. Actual CPU OBJ hashes
are verified; unchanged exports remain in the original raw directories rather
than duplicating195MB. Raw observer root:C:/Dev/tmp/horde-high-edge-witness-20261002.

Next unfinished step: independent contact reference, then smallest explicit
same-contact terminal/exit policy preserving opaque receiver and physical
transport. Do not repeat completed captures, sweep epsilon/precision, raise
traversal/cost budgets, omit the floor or promote either buggy baseline.
Keep High row43, Mobile finale, other High failures and physical/device gates
separate/open. No phone use while disconnected; no merge/release/publication.
Audio/haptic manual revalidation required:NO (shader-only, semantics unchanged).
