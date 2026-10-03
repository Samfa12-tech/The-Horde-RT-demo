# Staged-primary pass observer — separate investigation evidence

Normal Shipping and the untimed staged candidate remain unchanged. The optional
`HORDE_RT_STAGED_PRIMARY_TIMING` observer adds three timestamps per submitted
frame, collected only after its owning fence/idle completion. Missing, erroneous
or mismatched results remain gaps, not zero-duration samples. A bounded profile
report is separate from the accepted benchmark ledger; no generic telemetry or
render-scheduling framework was added.

## Validation and boundaries

- Current Shipping/Mobile MSVC Release app and six affected host tests pass.
  Timer/profile fixtures cover ordering, cancellation, unique ownership, wrapping,
  unsupported/error results, submission identity, bounded retention and JSON gaps.
- Shipping/Mobile Debug capture shell: all 13 standard captures plus production
  lantern are byte-identical to the retained untimed candidate. This is observer
  image equivalence, not a Release performance comparison.
- Real RTX5050 Release `lantern-held-high-v1` smoke completes its existing warm-up
  and measured laps. All 600 measured rows have exact query/benchmark submission
  identity, including final serial1200; no missing/rejected query rows. The scopes
  include primary prerequisite waits and the inter-pass barrier, respectively.
  This frozen desktop100% run validates observation, not phone performance,
  live physical transport or a matched control/candidate saving.
- First Release smoke failed because runtime assets had not been staged beside
  its EXE. Failure/capability logs remain retained. The single repaired run uses
  the existing packaging runtime roster:69 files verified against source hashes,
  with no editor/source music assets. No executable rebuild was needed.
- Four-ABI Android Shipping/Mobile benchmark builds pass. The first timing build
  exposed an armeabi-v7a typed-null move defect; it was repaired at the handle
  layer. A later Android-only failed-idle ownership guard justifies one further
  incremental build. Earlier failed logs and exact APKs remain retained.
- Packaged module inspection is bound to the actual final profiled APK and
  profiled Windows EXE. It does not borrow the earlier untimed Windows receipt.
  Normal four-module Shipping admission remains closed and unchanged; these
  eight-module investigation artifacts are not normal Shipping packages.

Exact artifacts, runtime JSON, module inspection, source fingerprints and compact
receipts are retained here or hashed in receipts under
`C:/Dev/tmp/horde-staged-rt-20261001/`. Builds used dirty source before their reviewed
commit; no retrospective clean-build identity is claimed. Unrelated scratch is
excluded from fingerprints and preserved.

Profiling runs must remain separate from unprofiled ABBA comparisons. Allocation
and logical write/read bytes are not measured DRAM bandwidth; RAM pressure,
cache misses, stalls, registers, spills and occupancy await exact-device evidence.
No device was connected or operated. No promotion, merge, release or publication.

The single [experiment record](../../ENGINEERING_1_6_1_STAGED_MOBILE_RT_2026-10-01.md)
owns completed results and the next unfinished step. Audio/haptic manual
revalidation required: **NO**; audio/gameplay semantic inputs are unchanged.
