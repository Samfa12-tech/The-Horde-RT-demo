# Bounded dielectric spawn correction — September 30

Latest bounded refinement: [derived-normal-bias-01](derived-normal-bias/README.md),
implementation `d63e29c`. It classifies/fixes row237 and clears ordinary RTX volume/
recovery events without changing physical budgets. Pipeline row43 and physical
phone/whole-glass gates remain open. The03c evidence below retains its own
historical source/artifacts/results; it is not relabelled as the newer pass.

Preceding candidate **03c**, based on pushed `c4169ac`, adds explicit non-finite
rejection to **03b**, its measured predecessor. The `cpu/` probes and
`runtime/03b-*` replay are investigation artifacts, not production entry points.
Accepted players, GLB/material authoring, ray masks, 41 counters and **4/8
interfaces / 2/4 volumes** remain unchanged. Reward-lantern GLB SHA-256 remains
`34a2522f2027d3fb04b77480cc929d36c5a19c6e0f33bf3fa0f0ae0959c99ec4`.
Phase 4 is **not accepted**; failures below are not suppressed.

## Proven cause and bounded change

[Native live witnesses](../2026-09-30-glass-live/rays/README.md) prove one valid
exit below 1 µm and two origins outside adjacent faces after binary32 transforms.
A global bias/minimum change did not repair all three. Barycentric renormalization
was rejected: 0.463/0.365 mm slides along narrow faces versus native metric-nearest
corrections 11.595/14.262/12.473 µm.

Loader flag4096 requires six supporting planes, opposite pairs, near-orthogonality,
convex support and measured residuals on **every** closed component using the
material. Closed tetrahedra/sheared/warped fixtures keep ordinary closed transport.
Separate minimum geometric width rounds down, measured geometry error rounds up.
Two measured widths are 0.9313 nm below binary32 authored 7 mm thickness: numerical
clearance uses geometry; authored optical thickness/IOR/attenuation stay unchanged.

The shader retains physical hit point/normal/UV and derives a separate spawn.
Its allowance includes reconstruction, both rounded affine transforms, inverse
residual, offset/inset construction and measured geometry error. Metric-nearest
conservative inset rejects corrections exceeding twice the derived error,
insufficient source/opposite clearance and non-finite inputs. Only an admitted
source-face-separated spawn uses zero query minimum; fallback retains 1 µm and
failure diagnostics. All intersections/continuations remain native RT, not an
analytic glass substitute. GPU material stride stays112 bytes; formerly unused
`.w` lanes carry loader bounds. Descriptor/counter ABI is unchanged.

This is **not** universal hardware-precision, disjointness, runtime-instance-contact
or nested-medium certification. Source/negative wiring tests and native witnesses
do not replace an executable arithmetic proof for arbitrary transforms.

## Current checks and exact artifacts

- MSVC Debug native build and **75/75 CTests pass** on03c; separate final
  artifact/compatibility and fresh eight-compute checks also **PASS** (exit0).
  Separate `c4169ac` fixes a no-op
  forged Diagnostic descriptor test (red/green1/1); push36661824032/PR36661828721
  CI green45 portable +11 Vulkan-host. Host coverage is not physical RT acceptance.
- Eight pipeline +eight compute variants regenerated/compiled/validated. Generic
  pipeline Diagnostic250488 bytes/62622 words/14827 instructions; Shipping241660/
  60415/14403. Both62 functions/198 calls/2 query sites/15 loops. Two added loops
  are fixed three-edge math, **not** traversal increases. Four Generic footprint
  ceilings increase3862 words; Opaque ceilings unchanged. Correctness cost, not
  performance gain.
- Actual ARM64 ELF extraction/disassembly/validation checks all four modules per
  APK. Diagnostic Generic retains41 atomics; Shipping pipeline/compute all
  **zero atomics/noBinding22/noimage readback**. See the two embedded-module
  disassembly summaries and containment logs under `checks/`.
- Immutable Debug APK (four ABIs), installed/pulled back identically onSM-S948B:
  `2dcd65b27768ca1a3b02375df9f20724849e006b1e795d78d2f5c1c98676c658`.
  Unsigned ARM64 Shipping APK:
  `a76fe241c77d7774bb9be6c74b189f0d3bb50bf4351aa84bc2b2f230789c966d`,
  builds/scans, **not installed/published**. Only ordinary Debug updated;
  stable/owner-accepted candidate apps/data untouched.
- Immutable High Diagnostic Windows EXE:
  `7ffbf2152653f3c859096666a84c98b875db1a2ad32fbef7d5e83652b0598790`.
  Both live processes have observed exit0. Thirteen standard captures have a
  closed/presented manifest; the earlier asynchronous capture command does not
  independently establish its process exit code.

## Changed paths and remaining failures

Owning RTX replay frame231/tick833 removes all three volume failures/recoveries
on both backends; all41 ordinary counters match. Retained03b manifests explicitly
say investigation-only, not ordinary checkpoint11 or performance proof.

Current03c live ledgers have **600/600** valid submitted/completed joins per
backend/device, zero rejects/cancellations and valid41-counter arrays. Every
03c array equals its respective03b predecessor; remaining failures are current:

| Exact run | Transport/pane events / rows | Volume budget | Mismatch | Primary certified recoveries |
| --- | --- | --- | --- | --- |
| RTX High pipeline,100%,1232×803 | 4 / 3 | 3 / 2 | 1 / 1 | 7 / 6 |
| RTX High compute,100%,1232×803 | 3 / 2 | 3 / 2 | 0 | 8 / 7 |
| SM-S948B Mobile pipeline,75%,1080×2235 | 64 / 58 | 57 / 52 | 7 / 7 | 299636 / 600 |

RTX mismatch row43/tick646; volume rows237/tick840 and454/tick1057 on both
backends. Phone firstfailure39/tick642, peak4at298/tick901, peakrecovery538at
181/tick784. Shadow overflow/unclosed/recovery0 **does not certify every shadow
case**. Benchmark pins Mobile shader bundle, not APK SHA; the installed/capture
receipt independently binds APK bytes. See current `phone/03c/live-*` and
`windows/03c-*`; predecessor data retains03b identity.

Versus the earlier ordinary ad6 live run, phone failures918→64, RTX107/105→4/3
support bounded correctness, not zero-error admission or matched Shipping speed.
Exact03c phone run125343 passes13replay/7captures/Home-resume, strictASTC/native
pipeline on **SM-S948B/Android16/Adreno840/driver2150932499**. All7PNGs/counter
snapshots equal03b: isolated overflow1/recoveries80, high0/527, grazing0/30702.
Physical paths remain open despite harness execution passing.

All13 current Windows standard PNGs equal03b. **Combined comparison FAILS**
unchanged timing gate (+4.067% vs2%): Diagnostic medians6.1859→6.43745ms are not
matched Shipping performance. Against the older geometric-shadow baseline,
finale still changes4pixels/max32 and fails unchanged pixel tolerance. Frozen
backend replay fails max channel error243 vs3 (30/989296pixels exceed1).
Only demonstrated ray defects are classified; other pixels stay open. Never
loosen tolerances or restore buggy baseline transport.

Phone video29.852s/251frames: retained six-sample contact sheet is game-only,
mostly startup/frame1 and first60warmupframes, **not measured-lap motion
acceptance**. Full video remains local. The600+600 ledger is separate execution
evidence; retained thermal context/Debug recording timings are not a matched
performance result. No new owner feel/audio acceptance.

03c phone context records thermal0→2, GPU thermal levels0–1 and battery33.8→42.1°C.
Diagnostic frame median273.0075ms/GPU261.0851ms versus predecessor03b167.0518/
155.9226ms is a substantial **regression warning** (unmatched thermal context,
not an attributable Shipping result). Do not ignore it or claim an improvement;
require controlled Shipping/warm comparisons after correctness classification.

## Next gate and evidence limits

Classify remaining exact live rows, genuine isolated Mobile interface exhaustion,
floor/contact and certified recovery semantics without budget/material/geometry
or diagnostic changes for counters. Initial-medium/both-inside/no-boundary shadow
attenuation stays open. Then affected images/continuous live glass; only afterward
matched Shipping performance and separate backend parity. S24/S25 unverified;
accepted player work stays closed. Audio/haptic manual revalidation: **NO** for
unchanged semantic inputs/backend/cues.

`artifacts.json` binds curated bytes/SHA-256. APKs/EXEs/full video/disassembly/
large OBJ dumps stay local. Earlier failures are retained honestly: first phone
attempt timed out120s (same03b APK passed300s); initial03c artifact check overlapped
a commit and rejected changed Git status. Final isolated checks are separate.
