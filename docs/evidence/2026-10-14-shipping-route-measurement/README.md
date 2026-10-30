# Sustained Shipping-shader chapter route

The chapter remains incomplete. **Normal static-region device-local residency
saving remains zero.** This checkpoint adds a reproducible Windows measurement
route and retains actual completed-frame evidence on both hardware RT backends.
It does not change production rendering, quality, capacities, materials, assets,
simulation timing, the existing fast capture or its assertions/deadlines.

## Measurement and limits

The isolated Debug executable selects the existing **Shipping/High** shader
pair with the supported CMake overrides. The evidence verifies Shipping artifacts,
absent Diagnostic counters and zero Diagnostic reads/resets. CPU code remains
Debug; these are not Release CPU/FPS acceptance results.

The normal two-way input probe starts after a staged Keeper defeat and first
ascent. It follows elapsed wall time, consumes complete60Hz ticks with fresh
input per tick, and retains debt behind a four-tick per-render bound. It never
seeds later progression or readiness. GPU completion identities join each
record to its committed input/route tag; repeated renders of one tick cannot
borrow a later tag. External probe simulation CPU time is separate from the
renderer-owned SimulationStep stage, which is zero in this harness.

Both runs cover landing sweep, fresh lantern raise/visible Kit, both bends,
lookout sweep, return, descent, lower sweep and repeat ascent. They preserve
once-only claim/deployment, presented reunion acknowledgement, equipment and
safe round-trip state. They do not test a fresh opening/guards/Keeper fight,
owner audio/haptic feel, phone execution or ordinary resource streaming.

RTX5050 Laptop GPU,960x540,100% scale, **Current shadows**, High water/fire,
High optical profile and dust Off. No settings are written. Actual GPU command
timestamps include RT rendering and recorded AS work; there are no separate
GPU skinning/AS/traversal/filtering timers. Skinning, upload and AS command-record
stages are CPU durations. Scene allocations are recorded memory classifications;
they are not OS process peaks or driver-wide VRAM usage. Power/thermal state was
not monitored; the receipt's P4/8.65W observation is explicitly post-run only.

Five initial seconds are excluded. Percentiles use nearest rank. The sealed
analyzer verifies serial continuity, identity/tag joins, canonical snapshots,
settings/artifact consistency and zero stage overflows before arithmetic.

| Completed evidence | Pipeline | QueryCompute |
| --- | ---: | ---: |
| Run duration |107.835s|106.123s|
| Presented/completed route frames |1,246|1,245|
| Samples after warm-up |1,208|1,207|
| GPU median /95th percentile |7.489 /13.765ms|7.386 /13.873ms|
| GPU maximum |15.585ms|15.573ms|
| Debug player skin median /95th percentile |22.592 /24.015ms|22.799 /24.039ms|
| Dynamic-upload CPU median /95th percentile |1.514 /2.745ms|1.484 /2.751ms|
| Image-acquisition CPU95th percentile |247.642ms|229.801ms|
| Whole render CPU-cycle median /95th percentile |54.623 /302.689ms|54.703 /284.332ms|
| Maximum retained simulation debt |1,615ticks|1,691ticks|
| Peak tracked device-local allocation |405,550,288bytes|405,550,288bytes|
| Peak tracked host-visible allocation |48,084,816bytes|48,084,304bytes|

The long acquisition waits and Debug skinning cost prevent interpreting these
GPU values as sustained playable FPS. No cause is assigned to acquisition stalls
without a visible/unoccluded presentation check. The next performance step is
an optimized CPU measurement with equivalent scoped controls and a verified
presentation environment, preserving assertions and quality. No production
optimization is justified solely by these Debug timings.

## Inspected actual candidate frames

All four PNGs were opened. Both backends show visible Kit, retained dressing,
terrain/tree shadows, the raised lantern, and the repeated-ascent scene. This
small gallery supplements the preceding full Diagnostic route gallery; it does
not replace that broader inspection or owner acceptance. Ground/tree material
contrast remains open, and the deferred canopy is unchanged.

| View | Pipeline | QueryCompute |
| --- | --- | --- |
| Initial frozen reunion |[Frame](native/pipeline/207-chapter-kit-reunion.png)|[Frame](native/query/207-chapter-kit-reunion.png)|
| Final repeated ascent |[Frame](native/pipeline/final-repeat-ascent.png)|[Frame](native/query/final-repeat-ascent.png)|

## Regression and export evidence

Clock and launch tests failed first; their logs are retained. Three affected
CTests pass in Debug and Release. Two existing completed-frame/serialization
CTest suites pass in Debug. Standard Debug/Release applications build, as does
the isolated Shipping measurement application. The actual Diagnostic executable
rejects measurement admission after its normal capture: no Diagnostic timing
record can be mislabeled Shipping.

The first two runs completed traversal but exposed invalid JSONL framing:
canonical pretty-printed objects crossed lines. Their original bytes, receipts
and source are retained under [failed](failed/). The correction removes only
actual formatting CR/LF; escaped string content remains intact. Both final runs
were repeated from the corrected executable and every exported line parses.
Analysis failures while checking initial field names are also distinguished from
renderer failures. No assertion, sample, capacity or overflow was hidden.

Final source inputs/artifacts are in [the receipt](source-and-artifact-receipt.json).
The measurement executable SHA256 is
`213a49a4fbc2f524e9cb03aa465a4b4d87c798ca144d3c3dfe39104f7ab41a09`.
Standard Debug SHA is
`99c7772804d90d23d0a5431603a1776284020e5fb1f338d317b30960023ef8f2`;
Release SHA is
`b1535ec3d9286bd5ed9422f80bff614d61abee91f7f4a027fc8b552952047d08`.
These are pre-commit builds against861f8331 plus the recorded input hashes.

The prepared phone APK remains byte-identical:
`f9d01fc4f53ca75719fbfca9cc82d3afcfaee5e1a2992e9bd41b4c52671370a2`,
183,145,434bytes, versionCode10. No Android source, package or device changed in
this slice. Exact-commit CI is recorded on PR27 after pushing; local Android
build/lint/Java/shader/package gates are not repeated for Windows-only tooling.

To reproduce, configure/build the isolated Shipping target using the command
in its launch receipt, then run `tools/measure-chapter-shipping-route.ps1` with a
fresh `-Output` (add `-Query` for QueryCompute). The process bound is180seconds;
the measurement clock bound remains150seconds. From this packet, arithmetic can
be reverified with `python analyze.py native/pipeline native/query`; this reads
sealed files without overwriting them. This uses Horde's native evidence format,
not a `game-dev` adapter certification.

Normal contributor-aware GPU preparation/loading/retirement, optimized sustained
throughput, Higher-shadow sustained costs and fresh full phone/owner acceptance
remain required. Current/Higher and saved Lower-to-Current phone tests remain
pending. Easy Tree/canopy and mist issue28 stay deferred.

Owner route: opening/guards → Keeper → claim → rope → fresh lantern raise/Kit →
both bends/lookout → return/descent → second ascent. Note transition hitches,
corpse/equipment/light continuity and speech/cue intelligibility; repeat the
affected graphics checks on the phone when it is available.
