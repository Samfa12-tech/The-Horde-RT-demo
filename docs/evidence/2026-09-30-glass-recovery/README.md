# Remaining glass recovery isolation

Base runtime: `632322d7a5e1901c732d6d012adaeaa936737d6c`. Accepted corner/Tmin
and duplicate-candidate fixes and all player assets remain unchanged. These are
**investigation captures, not a glass acceptance or performance pass**.

## What the remaining counters actually mean

An investigation-only Diagnostic/Mobile shader marks opaque certified terminals
red, misses green, certified open-volume budget recovery yellow, and independent
interface-budget overflow blue. No transport decisions, limits, material, masks,
geometry or diagnostic increments change. The top six rows are reserved for
float32 records from selected rays. Native `CaptureStorageImage` preserves raw
RGBA bytes; Android display screenshots change marker/record colours through
colour management and cannot decode the records reliably.

Exact SM-S948B, Android16, Adreno840, driver2150932499; pipeline backend, ASTC,
75% /1080x2235. APK `fd41de34ca80f015423ec016a8335760280da8052720091ca70063a156c09c8b`
was installed and pulled back byte-identically. Run073651 passes three capture
and Home/resume execution checks; remaining transport failures are still failures.
The source is deliberately dirty with the retained probe patch, not a clean release.

| View | Opaque terminal | Miss | Open certified budget | Closed independent budget |
| --- | ---: | ---: | ---: | ---: |
| isolated lantern | 0 | 0 | 80 | 1 |
| grazing fixture | 30081 | 0 | 0 | 0 |
| high/look-up | 1 | 0 | 527 | 0 |

Decoded records and original storage bytes are in `phone/paths`; original state,
manifest, installed hashes and capability JSON bind the records to their artifact.
`probe/decode-recovery.py` reproduces decoding/counts without editing the images.

- Isolated pixel(346,1200): entry19, TIR58, TIR20, exit21, then next pane27.
  Four real interfaces have already been processed when the fifth is encountered
  with no medium open. Remaining throughput is about0.704. This is **actual fixed
  Mobile budget exhaustion**, not another skipped exit or negligible contribution.
- Isolated pixel(340,1046): entry55, TIR56, exit57, entry63, then exit61 at the
  budget boundary with volume open. Remaining throughput about0.634. The80 yellow
  pixels have not all been individually replayed; this representative explains
  why a certified recovery can also be real finite-path exhaustion.
- High pixel(427,1520): entry61, TIR60, exit63, entry45, then exit7 at the budget
  boundary. This representative is also actual interface exhaustion.
- Grazing pixel(638,1216): entry11, TIR1/5/7, then floor486 at y=-.94999993 while
  glass remains open. The fixture bottom is coincident with that floor plane.

Separate RTX5050 Laptop Diagnostic/Mobile pipeline proof,540x960, uses the exact
baseline EXE SHA `f8588dc8b2b53d4c37593aa70681e203ef5e96300cc9572e057fb146603f3940`.
The baseline has1806 certified recoveries; marker capture contains exactly1806
red pixels and no other markers, with zero changed unmarked pixels. Its selected
pixel(328,842) records entry6, TIR0, then floor486 at y=-.94999951. The fixture
bounds are x[-9.2,-9.0], y[-.95,.30], z[-15.575,-14.825]. This proves **one real
glass/floor contact**, not that every opaque terminal is legitimate. Do not globally
reclassify open-volume opaque terminals: that would conceal the next defect.

## Demonstrated hit-position precision defect

High/look-up pixel(193,1537) records glass cap59, then floor486 after2.718852m,
with no TIR and the volume still open. An additional exact phone query probe
uses unchanged geometry, outgoing Snell direction, current30.090um normal bias
and1um minimum. Its triangle vertices/barycentrics are recorded in slots5–7;
the five alternate next-query results are slots8–12 in `phone/position`.

| Query start | Normal bias | Next hit |
| --- | ---: | --- |
| origin+direction*t | 0 /2 /8um | floor486, about2.719m |
| triangle barycentric surface point | unchanged30.090um | glass57,1.923732um |
| triangle barycentric surface point | 2um | glass57,3.503828um |

Ray-t point: (-11.2752857208,.7012073398,-15.0450563431).
Barycentric point: (-11.2752857208,.7012071609,-15.0450534821).
The2.861um Z difference exceeds the short exit scale. Runtime triangle59 has
local+Y normal; triangle57 side normal is(.5,0,-.866025), orthogonal to it. Bias
size alone does not fix this ray. Surface-point reconstruction is the demonstrated
layer to repair; it does not justify reducing Tmin or changing materials.

Exact position-probe APK `b222efb1abf5a9ee00760e727a1276355cbbc5fefb5d2c86c5afa29bdf0b6404`,
run074801, installed/pulled back identically, capture and lifecycle pass. Its
SPIR-V is `76368574ef503a6e0efa03b18e778100d471762b73f0992169eba912020f9451`.
This is query evidence for one selected path, not an already validated production fix.

## Probe containment and next work

Retained source-only patches reproduce the temporary marker/record/native-dump
instrumentation. They are **not production code**. Investigative SPIR-V validates
and disassembles but exceeds the production frozen footprint; those budgets were
never loosened. Shipping/compute artifacts and physical4/8-interface limits are
unchanged. Original production source/artifact paths are restored before commit.
Ordinary Debug APKea1c6265 is restored and pulled back identically; run075136
passes the three captures/Home-resume. Both isolated/high PNGs are byte-identical
to the retained accepted-corner images; the third grazing view also executes.
`phone/restored` retains the installed identity and manifest. Stable/candidate
apps remain untouched, and no temporary probe is the current installed build.
Build logs/artifacts remain locally under `reports/glass-recovery-*` and
`C:/Dev/tmp/horde-glass-recovery-20260930`; stats and exact captures are retained here.

Next: transmission-only triangle-surface position correction, meaningful regression
and exact phone/RTX images; then geometric shadow correctness and live glass.
The actual fixed-budget failures stay explicitly open. No budget increase, free
TIR interfaces, suppression, scalar shadows, geometry/material change, restored
buggy pixels or pixel-tolerance relaxation is proposed. Glass/floor contact needs
correct physical boundary handling, not blanket opaque-terminal acceptance.
Matched Shipping performance and pipeline/compute parity remain later gates;
neither is certified by Diagnostic probes. S24/S25 remain unverified.
Audio/haptic manual revalidation required: **NO**; feedback is unchanged.
