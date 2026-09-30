# Exact SM-S948B Shipping comparison and opening-room investigation

September30,2026. This is measured performance/investigation evidence, not Phase4
glass correctness, final release acceptance or publication authorisation.

## Frozen pair and experiment boundary

A: source d504bfed51976e71e496ead81dbd284bec03da33 (03c), APK
`13a8676998c14fed139a14174757a5b1aecb48e50b902834799ab4c970875cf3`.
B: source 8cbe0ac44d4fb3a168ca891985ca560b43fe4541 (runtime d63e29c), APK
`b5a4344b6f860259d88a4d7bae3d349f462704c9f5a689773626f8a93dc39c6f`.
Same isolated development-signed `.benchmark`, ARM64 RelWithDebInfo,
non-debuggable/testOnly, Shipping/Mobile shaders, checkpointsOFF. Each swap was
installed/pulled back byte-identically without clearing data or changing the
stable/owner-accepted app. Device SM-S948B/R5GL219SZGK, Android16, Adreno840,
driver2150932499, native RayTracingPipeline, strictASTC, MAILBOX,
75%/1080x2235 internal and1440x2980 presentation. No video, resolution reduction,
quality reduction, player/material changes or fake RT.

Actual packaged modules validate/disassemble:0diagnostic atomics,0imageReads,
noBinding22. Independent asset/module receipt retains all53 payload comparisons:
50byte-identical, remaining3text files differ only in formatting (parsedJSON or
line endings). OpaqueFast instructions are identical after debug metadata removal
and bijective numeric-ID renaming; generic transport differs. Inspect the receipt
and matching script rather than inferring equivalence from filenames.

Each trial uses the existing whole warm-up lap plus full measured lap:
route1838+1838, held-high/live600+600. Every admitted report retains all exact
submission/completion/CPU-index joins, valid GPU timestamps, successful real RT
presentation, zero failed/rejected/cancelled rows and compiled-out/null diagnostics.
Shipping null counters do NOT prove that the recorded Diagnostic glass failures
disappear. CPU cycle spans render entry through present return, not actual display
intervals or input latency. GPU command-buffer timestamps include AS/RT/copy.

## Repeated measurements

Values are native render-cycle median/p95 milliseconds, not live/display FPS.
Blocks run route, held-high, live-reveal in that order, re-installing the immutable
member before each block. All full-ledger integrity checks PASS.

| Block | Ordinary route | Held-high | Live reveal |
| --- | --- | --- | --- |
| A1 | 58.811/79.280 (cooldown-spanning) | 237.826/239.655 | 247.944/256.137 |
| B1 | 60.197/80.442 | 225.085/242.036 | 236.466/250.591 |
| B2 | 60.224/79.593 | 225.574/238.987 | 236.824/250.598 |
| A2 | 62.367/83.991 | 239.220/250.798 | 248.883/263.421 |

Owner added an external ice brick around06:49UTC. A1route crosses cooldown and
is excluded from causal ordinary-route comparison. Preserve it, do not discard
or pool it into a gain claim. Other route samples start29.1–31.1C/end32.6–33.0C,
thermal0; held-high32.4–32.9C/end34.3–34.8C, thermal0–1; live31.2–35.0C/
end34.7–35.6C, thermal0–1. These are post-launch observations, not frame-aligned
measurement boundaries. GPU clock reads are permission-denied; thermal/power
state overlap cannot establish identical clocks. Cooled measurements cannot
certify sustained uncooled gameplay.

Descriptive mean-of-run-medians: held-high238.523→225.329ms (−5.53%),
live248.414→236.645ms (−4.74%). This is consistent direction in these repeats,
not a causal gain or30FPS acceptance. Broute medians give16.61FPS, held-high
4.44FPS, live4.23FPS; no relevant path approaches the33.333ms/30FPS minimum.
The ordinary-route A1confound prevents a clean gain conclusion there.

Uncooled B0context is retained separately: route60.2439/79.8899ms,
GPU50.0545ms, battery36.2→39.5C/thermal0–1; held-high238.7385/248.8321ms,
GPU236.8249ms, battery42.3→43.5C/thermal2–3. These are initial observations,
not an uncooled steady-state soak or matched regression attribution.

## Actual opening strategy, separate from timing-pair identity

Minimal existing-metadata export fix8fbd46c retains owning activeStrategy per row,
without new shader instrumentation/collection. Exact source eafbf82 export APK
`ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae` installs/
pulls back identically AFTER the frozen ABBA sequence. Its four shader hashes
equal B; do not relabel its CPU timings as frozen-B measurements.

Fresh1,838/1,838 valid completions prove OpaqueFast for the entire ordinary route,
including160/160 opening frames. Opening native cycle median77.7652ms/p9591.4436;
owning GPU-command median66.4793ms/p95(nearest-rank)79.8930. None of160 GPU
intervals is within33.333ms. Route player CPU skin median8.7267ms, scene upload
0.3429ms, fence wait49.9471ms (route aggregates, NOT opening-specific CPU values).
This rules out accidental generic-dielectric selection as the ordinary-route
explanation, but does not identify torch/shadows as the cause.

## Temporary GPU-stage probe and its admission

Detached source18616f4 plus investigation-v3.patch, APK
`cbfc5cf40d15b8a7c64561d4eff11a2dbf0a9dd9c7179da9a53850ef74905204`,
adds only opt-in, local
benchmark GPU timestamps. It does not ship or change GPU shaders/geometry/quality.
Two independent existing timers measure AS-build and dispatch intervals with
stage-specific endpoints. Query state follows the exact submitted identity and
owning successful fence/idle; unsupported/failed waits have explicit non-duration
status. Normal frame timer TOP/BOTTOM semantics are unchanged and tested.

Probe v2 APKdfd5c35e… was rejected by its stage admission: the last scheduled
frame lacked probe timings because Advance() had already finished the mutable
run. Normal1,838-frame benchmark evidence remained valid. v3 uses the already
captured owning-frame flag and must join every measured stage to all seven exact
identity fields. Zone/activeStrategy come from that immutable completed row,
not mutable state at fence completion. Retain v2 and the v2→v3 patches; no count
or tolerance was weakened. Ordinary A/B artifacts are untouched.

Vulkan stage timestamps are execution-dependency intervals and may be written
at a logically later stage, not isolated-engine counters or necessarily additive
components ([Vulkan specification](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdWriteTimestamp.html)).
Queries and host logs perturb the probe, so it is separate from matched Shipping
frame-time conclusions. AS excludes CPU preparation; dispatch still includes
all lighting/shadow/fire/traversal. A further controlled isolation is required
to attribute torch/shadow cost.

v3 admission PASS:1,838/1,838 measured frames join both stage intervals through
all seven identity fields, including the final scheduled frame. Opening160/160
OpaqueFast frames: GPU AS median0.4956ms/p95(nearest-rank)0.5906,
dispatch65.6335/72.9296, full GPU-command66.2730/73.5807. This directly separates
GPU geometry-update cost from dispatch: updates are not the dominant opening
GPU cost. It does not exclude player-geometry traversal inside dispatch or
identify lighting/shadows as the cause. Do not subtract/add aggregate medians as
if they were synchronous engine counters. No real renderer optimisation was
promoted by this probe.
v3 post-launch context22.3→27.6C, Android thermal0/GPU thermal-power0, under
external cooling; it is not thermally matched to the export-only control and
its native frame time must not be used as an optimisation gain.

## Targets and gates still open

Maintain the30FPS ordinary/lantern-heavy engineering target at unchanged75%,
with recovery towards45–60 only when measured headroom supports it. No honest
current evidence supports claiming that target achievable in this candidate:
ordinary opening needs major GPU-cost reduction; heavy lantern needs severalfold
improvement. Optimising CPU player skin alone cannot meet a33.333ms budget while
the opening GPU interval is66ms. This is an inference from timing bounds, not
proof that player geometry traversal or torch lighting is inexpensive.

Preserve physical glass interface/recovery/live/attenuation/changed-image gates,
separate backend parity, exactS24/S25 (unverified), warm uncooled sustained and
actual display pacing, final Windows/Android matrix and owner-only licence/
signing/publication gates. No completed Phase4 claim. Audio/haptic manual
revalidation:NO (no semantic feedback/playback changes). Deferred owner-requested
live benchmark FPS counter remains later in1.6.1, not part of this frozen pair.

## Reproduction and provenance

SHA256SUMS.json describes the retained allowlist. Full raw reports are losslessly
gzip-compressed; source/decompressed SHA256 and lengths are recorded and verified
by curate-evidence.ps1. Decompress each benchmark.json.gz beside its result.json
before rerunning analyse-trial.ps1. Never edit raw reports. Native text summaries,
exact install receipts, complete thermal samples, build/containment checks,
derived analyses and scripts are retained. Actual APK/ELF/SPIR-V binaries remain
external/local; no arbitrary media or private phone/Home captures are included.
The probe logs are filtered only to explicit OPENING_GPU_STAGE markers; the exact
identity join is reproducible with analyse-opening-stages.ps1.

Current18616f4 push36684658361/PR36684662870 both pass45portable+11Vulkan-host,
fresh logs inspected. Minimal export tests pass MSVC Debug5/5 andRelease5/5;
temporary stage-parameter timer test passes Release1/1. Android ARM64 benchmark
and probe v2/v3 builds PASS; these are not a four-ABI/final-candidate matrix,
Windows RTX acceptance or phone RayQueryCompute proof.
