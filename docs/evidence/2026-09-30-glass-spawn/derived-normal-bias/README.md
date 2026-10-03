# Per-hit normal separation — September 30

Implementation: `d63e29c7f119a180ce2a6504831d501e8ba0a95e`, based on
`3140aca9ae528df65766bda6f63a2bfe28ba742d`. Candidate **derived-normal-bias-01**.
The first host checkpoint preceded phone reconnection. Its evidence below closes
the demonstrated row237 numerical defect, **not Phase 4 or phone acceptance**.
New exactSM-S948B evidence is separately identified in the follow-up section.

## Proven cause and correction

The prior ordinary replay identifies two rays at (515,569)/(515,570), row237,
tick840. Retained native witnesses trace their actual queries; all41 counters
match the ordinary replay. No alternate analytical query replaces traversal.
Both first hits are triangle62/component4, instance8/material115. Rectangular
eligibility is true, but the guard rejects at source-face normal separation:

| Pixel | Lower normal clearance, local metres | Upper error, local metres |
| --- | --- | --- |
| 515,569 | 0.000068532630394 | 0.000069284062192 |
| 515,570 | 0.000068532630394 | 0.000069649264333 |

Opposite-face usage0.0007115/0.0007117 remains below0.001749366. The rejected
guard falls back to a spawn outside sideface63 by0.265/3.259µm in the **captured
native object ray**. Native next hit is foreign component0/triangle3 while the
source medium is open. Double intersection using those captured float inputs
matches that foreign hit; an ideal world-ray reconstruction is not this proof.
The immutable GLB remains `34a2522f2027d3fb04b77480cc929d36c5a19c6e0f33bf3fa0f0ae0959c99ec4`.

The guard now derives a per-hit minimum normal bias from its lower projection
scale/upper reconstruction error, with conservative inflation and one-ULP upward
rounding. An absolute matrix-vector/dot error bound covers cancellation. Only a
successful guard publishes the minimum; both reflection and continuation origins
use their matching hit's minimum. Fallback and its diagnostics remain unchanged.
The250µm cap, opposite-face/inset guards, independent query minimum, all4/8
interface and2/4 volume budgets, optical material, geometry, masks, player pose,
descriptors and41-counter ABI are unchanged. No analytic RT substitute.

The separate instrumented **r1** candidate captures minima30.4850/30.6457µm.
Its actual next origins are inside every source halfspace (at least58.70/61.55µm
local clearance); native triangle27 exits are42.6314/44.7162µm away. Following
native interfaces have cosine0.60566–0.60651 versus critical0.75311 at IOR1.52:
genuine TIR. Seven processed TIR events per ray precede the queued eighth exit
at the unchanged eight-interface ceiling. Failures/recoveries disappear, TIR
counts2062→2076 and bounded TIR terminations258→260; no diagnostic suppression.
Outside four output-witness rows, only the two affected pixels change, max22.
That is demonstrated corrected transport, not permission to loosen image gates.

**r1 predates the final absolute-projection bound.** Its source patch/modules are
investigation-only, not the exact final binary. All probe sources are removed.
Final ordinary source/modules are independently exercised in the ledgers below.
This is not universal cross-vendor/subnormal/traversal-precision certification.

## Final ordinary evidence

RTX5050 Laptop, High100%,1232×803, existing gameplay-owned
`lantern-reveal-sequence-v1`,600 warmup +600 measured steps. Both owned native
processes exit0;600/600 valid41-counter rows, matching submitted/completed
identities, monotonic serials, no rejects/cancellations. No phone inference.

| Ordinary backend | Transport/pane mismatch | Volume/interface failures | Certified recoveries | Shadow failure/recovery/unclosed |
| --- | --- | --- | --- | --- |
| RayTracingPipeline | 1 at row43/tick646 | 0 | 0 | 0 |
| RayQueryCompute | 0 | 0 | 0 | 0 |

Prior03c volume failures3 and recoveries7/8 are now0 on pipeline/compute. Row454
also clears in the ordinary ledger; its cause was not separately witnessed.
Pipeline-only row43 remains to classify. Zero shadow counters do not prove
unknown-initial-medium/both-inside/no-boundary absorption or live visual acceptance.

All13 standard pipeline captures complete/honestly present (owned exit0); PNGs
are byte-identical to03c. The unchanged combined image/timing comparison passes;
Diagnostic timing−0.308% is **not matched Shipping performance**. Older affected
finale/backend image failures remain open at their unchanged tolerance.

MSVC Debug build and focused CTests4/4 pass: dielectric math, static GLB,
production prop and raygen manifest. Full artifact/negative-source/compatibility
checks pass, as does a fresh independent eight-compute check. New tests enforce
finite strict separation, unchanged cap, upward rounding and actual use at all
four origins; an unused helper cannot satisfy the wiring checks. No claim that
the older full75-test run is this candidate's full matrix.

All16 shipping/diagnostic variants freshly compiled/validated. Generic pipeline
Diagnostic252680B/63170words/14955instructions/830branches; Shipping243852B/
60963words/14531instructions/783branches. Both63functions/199calls/15loops/
2query sites. Correctness cost versus03c: +548words/+128instructions/+9branches;
only four Generic footprint ceilings rebased. Opaque statistics/ceilings and
physical traversal budgets unchanged; source-identity metadata changes do not
imply byte-identical Opaque SPIR-V.

Android ARM64 Debug and unsigned Shipping build successfully. Actual four native
modules per exact APK validate/disassemble against current catalogs. Shipping
has0 atomics/0 image reads/noBinding22; Diagnostic Generic retains41 atomics.
See packaged summaries and containment logs. Neither APK had been installed at
the host checkpoint; the later Debug installation is recorded below.
No signing/publication or owner-accepted app/data changes. Audio/haptic manual
revalidation required: **NO**, RT-only with semantic inputs/cues unchanged.

Exact local artifacts (not committed binaries):

- Ordinary Windows EXE: `c9d86f61bd4a81e271ebc3cc1909ae7c754470e8b4e8e3cbb04056768fdd3b78`.
- ARM64 Debug APK: `389d6954f7a3fddde1ced690470029fa4b14cc6f825fa34bf7269dc617f946e0`.
- ARM64 unsigned Shipping APK: `7a03f076e6a24768d87258cf9c95f3d09bbc09803d3239ca96f696e22b250bd8`.
- First native-ray probe EXE: `27a7d8bdd2c4d36e57581c13b65f240ea6d683e2f6edac8e07e520f61c05a400`.
- Detailed guard probe EXE: `476eaa6ee018809ca748b9342350b159b82a059e62a1e9e4eaaab4a23780e0b8`.
- r1 bias probe EXE: `72dd3daf44491a7e0ff5dc50caeaaff62a6fa756acd4b52e13adcb330e48e717`.

## Reproduction and remaining gates

`rays/decode-remaining.py IMAGE FIELD_COUNT` supports83/106/107 fields for the
first/detailed/r1 captures respectively. `rays/analyze-native-paths.py READER GLB
WITNESS` reuses `../../2026-09-30-glass-live/rays/probe/analyze-volume-geometry.py`
(relative to this directory); provide the immutable GLB and retained decoded
witness. Its double arithmetic consumes native float inputs; it is not a GPU
transport or performance substitute. Source patches bind each output-row format.
Large OBJ dumps, full disassembly and executable/package binaries stay local.
The parent `artifacts.json` hashes the curated files.

## Phone reconnection and row43 follow-up

Evidence source checkpoint25a379c; exact DebugAPK389d6954 above, built before
Windows-only probes and independently module-matched to d63e29c. Runner records
`sourceDirty:true` honestly; scratch/probe state is not relabelled a clean build.
ExactSM-S948B/Android16/Adreno840/driver2150932499 installs/pulls back identically.
Run152111:13replay/7capture/Home-resume PASS, strictASTC/nativepipeline75%/
1080x2235. Stable and accepted candidate apps/data untouched; Shipping uninstalled.

| Phone evidence | Result, not whole-glass acceptance |
| --- | --- |
| Isolated lantern | interfacefailure1, recoveries80; changed PNG remains unattributed |
| Raised/low-parry/grazing | failures0; recoveries527/0/30702 respectively |
| Millimetre/tinted/fire | failure/recovery0 in these captures only |
| Live600warm+600measured |600/600 valid41-counter rows, exact joins,0rejected/cancelled |
| Live transport |12events/12rows:5volume-budget,7mismatch; interfacefailure0 |
| Live recovery |299513events across600rows; reasons remain explicit |
| Live shadow |failure/recovery/unclosed0, not universal segment proof |

Six of seven capture PNGs equal03c; isolated image changes3179pixels/max187,
at[y1523..1784,x455..658]. The unchanged pixel gate remains open: corrected
transport is plausible but these changes have not all been attributed. Live
volume failures57→5 versus03c is correctness evidence, not a platform-wide pass.
Seven live mismatches remain. Existing counter definitions/budgets are unchanged.

`phone/live-benchmark.json`, result/context/analysis retain exact ledger/settings
and thermal sampling. Diagnosticmedian329.7271ms/p95349.7533ms, GPUmedian319.7446ms;
thermal0→2/battery35.8→43.3C. These recorded, unmatched Debug runs do not establish
Shipping performance or explain away the prior major regression warning. Median
is slower than all16.667/20.000/33.333ms reference bands. Matched Shipping remains
required after physical correctness.

Only two visually verified in-game frames (warmup/measuredlap frame420) are
curated. Third recording continued onto Android Home after benchmark export;
that video/private frames are excluded and preserved local-only, not displayed,
committed or counted as motion acceptance. Raw videos stay local. This is not an
exhaustive live visual or owner-feel pass. Phone returned Home after completion.

[Row43 native evidence](row43/README.md) narrows the remaining RTX mismatch to
outside-origin corner edge/arithmetic sensitivity. No source correction is
claimed. Probe sources/modules are removed; production byte identity restores
25a379c. Restored native Debug build and focused2/2 pass; eight compute variants
regenerate identically. Source25a379c push36672645385/PR36672649224 CI are green
45portable+11Vulkan-host (not hardware acceptance).

The retained decoder's83-field secondary-row stride is repaired to48;106/107
remain80. All three historical formats are checked against retained JSON.
Next isolate Mobile interface exhaustion/contact recoveries and remaining live
events, geometric shadows and changed pixels; then matched Shipping/backend
parity. Keep S24/S25 and phone compute unverified, accepted players closed, all
later programme scope intact. Audio/haptic manual revalidation:NO, RT-only.
