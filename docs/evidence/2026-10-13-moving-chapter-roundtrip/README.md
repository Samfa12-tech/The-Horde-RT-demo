# Moving chapter route checkpoint

13 October2026; implementation parent48de62a4. **The chapter remains incomplete,
and normal-play residency saving remains ZERO.** This slice adds an input-driven
validation route; production geometry, shaders, budgets, quality and assets stay
unchanged. [Exact source and artifact receipt](candidate-receipt.json).

The probe starts after the existing staged Keeper death and actual first rope
ascent. It walks back to the landing, sweeps360degrees, returns to Kit, issues
one fresh lantern raise, waits for the real successful-presentation authority,
inspects the available walking clip, follows the native terrain to the lookout,
returns, descends, sweeps the lower landing, then ascends again. Every motion uses
ordinary axes/context edges and authoritative60Hz steps. It does not import
positions after startup, seed readiness or manually accept the reunion in the
native run. It rejects stale generations, failed readiness, duplicate ownership,
nonfinite player state and stalled phases. Pause emits neutral input. The host
fixture's explicit presentation acknowledgement is CPU-only evidence.

Use `tools/capture-chapter-roundtrip.ps1 -Output reports/fresh-name [-Query]`
after a Debug build. It opens only its scoped, nonactivating capture window,
sends no desktop input, changes no saved settings and mutes only process-local
audio. Output and receipt prefixes must be fresh. Release rejects the flag.
The helper now records executable/source hashes before launch, including failed
startups. Both its180second deadline and the native150second run bound remain
fixed. Captured images are raw RT storage, without HUD/subtitle overlays.

## Actual RT route and limitations

RTX5050 Laptop GPU, Debug Diagnostic/High/Higher,960×540 at100%, with all admitted
effects and full geometry retained. The scripted render cadence explicitly
batches **up to four60Hz simulation ticks per frame**. This preserves every
simulation step, edge and collision; it is not a Shipping performance test or
owner assessment of animation smoothness/input latency. Exact counters assert
that the platform cannot advance the already-owned simulation a second time.

The final Pipeline run completes **4,961ticks,1,241rendered frames and26captures**.
Its final grounded upper landing has the lantern and sword unstowed, one claim,
one deployment, a successful fresh raise, descent/landing witnesses and repeated
ascent. The gallery records exact cameras, Kit state, rope/equipment state,
completed submission/presentation identities, active shader hashes and resource
inventory. The landing compass views, both bends, Kit walking, lookout panorama,
lower views and second landing were opened and inspected. Continuous ground is
visible behind the rope; niches/tomb, sparse prepared trees and town remain the
existing art. Easy Tree/canopy work remains deferred; this is not finished forest
art or bespoke Kit acting acceptance.

The **final QueryCompute run is incomplete**: it presents1,123frames and4,492ticks
through the lower-landing sweep, then reaches the unchanged180second process
deadline before repeat ascent. Its partial JSON/CSV and frames are retained as
failure evidence. Process cleanup is not Vulkan retirement proof. An earlier
Query prototype completed the round trip before the final one-second Kit walking
inspection/metadata addition; its executable hash was not sealed before rebuild.
It is retained historical evidence, **not a pass for the final executable**.
Final QueryCompute repeat ascent remains pending; no deadline was increased.

| Final-run evidence | Pipeline complete | Query incomplete |
| --- | ---: | ---: |
| Render-cycle CPU median /p95 |58.46/73.13ms |64.66/103.32ms |
| Owning moving GPU mean /p95 |7.81/13.69ms |8.45/13.88ms |
| GPU samples joined to moving ticks |1,241 |1,122 |
| Device-local allocation |405,550,288bytes |405,550,288bytes |
| Host-visible allocation |49,408,256bytes |49,407,744bytes |

CPU simulation batches and render cycles include large Debug/host stalls; maxima
and raw rows are in the receipt. GPU durations belong to their own completed
submission, joined by tick/serial; the initial frozen sample is excluded and the
Pipeline final completed sample is included. The failed Query tail is not
invented. GPU command-buffer timing includes RT/update/copy work; these numbers
do not isolate traversal alone, prove sustained Shipping performance, establish
a phone budget or certify streaming. No scale/effect/capacity was reduced.

## Inspected gallery

![Landing behind the rope and open shaft](windows/pipeline/3-landing-sweep-2.png)
![Visible Kit using the admitted walking clip](windows/pipeline/6-kit-walking.png)
![Prepared trail bank and bend](windows/pipeline/9-outbound-5.png)
![Lookout and distant Bellwether shell](windows/pipeline/11-outbound-7.png)
![Returned lower landing](windows/pipeline/21-descent-2.png)
![Second ascent restores held equipment](windows/pipeline/26-repeat-ascent-2.png)

[Full Pipeline gallery and exact frame evidence](windows/pipeline/gallery.json),
[raw moving rows](windows/pipeline/frames.csv),
[incomplete Query rows](windows/query-incomplete/frames.csv).
The incomplete Query gallery is intentionally preserved as raw partial JSON.
All26Pipeline frames and representative Query frames were opened. The25common
views are visually consistent, but are not byte-identical; the largest comparison
changes10,626of518,400pixels. [Raw pixel comparisons](backend-pixel-comparison.json)
retain channel deltas without treating this as final backend/owner acceptance.
The start of descent has a large dark foreground shape during stowing; its
ownership and owner feel remain a focused follow-up, rather than an attributed
rendering fix. Later lower/upper landings restore ordinary held equipment.

## Tests, failures and remaining integration

Initial host RED has three failed assertions. Final native-terrain CPU coverage
passes22,679cases, including4,962probe loop iterations, pause, failed readiness,
already-raised start rejection and stale reconstruction. Debug passes the legacy
rescue and authored-terrain CTests in114.08s; Release passes both in11.51s.
Windows Debug/Release applications build. Invalid standalone flags reject with
exit2; Release rejects the Debug-only mode. The reusable helper parses cleanly.
CI already selects both required CTests; exact-commit CI is checked after push.
No Android source/payload changes in this slice; its exact prepared APK remains
`1a6f553f9f5761e7d4979ae0fc5b029ca46fe07f4a9bf648be89c71c758f8abc`, uninstalled.
The preceding48de62a4 four-ABI/lint/Java/package and full1,475legacy+1,812roof
torch results retain their scope; no original assertion or180second policy changed.

Native prototype r01 reproduced duplicate stepping: `RenderFrame` called the
platform `AdvanceFrame` after the probe's direct step. An explicit scoped owner
flag now disables only that platform advancement, and actual before/after tick
counts guard it. r02's one-tick-per-render trial exceeded the capture deadline.
r03's explicit four-tick batching completed both backend prototypes; the final
r04 Pipeline completes, while r04 Query times out as recorded above. Failed
logs and raw partial rows remain retained; none is treated as harmless or passed.

Contributor review still blocks automatic tomb omission. Primary/transmission
rays reach10,000m;12m secondary rays start at reachable hit points, and local/fire/
moon visibility has separate source domains. A camera-only roof/volume test does
not prove these contributions absent. The full geometry stays resident. A
conservative policy over actual opaque geometry/all applicable origins, two-way
normal GPU readiness/adoption/completed-work retirement and sustained Shipping
measurements remain required. No normal replacement/eviction is claimed here.

Next owner route: defeat the Keeper, claim the lantern, climb, look behind the
rope/down the shaft, raise the lantern fresh to Kit, walk both bends to the
lookout, return/descent and ascend again. Check Kit idle/walk/wait, voices/subtitles,
lamp/equipment restoration, collision and any hitch. Physical Android full route,
Current/Higher and persisted Lower→Current, orientation/background, audio/haptics
and final owner visual acceptance remain pending. The shared phone is reserved
for another authorised test; Horde has no active device work. PR27 remains draft
with base conflicts; no merge or release.
