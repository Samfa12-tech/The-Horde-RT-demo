# Bounded TLAS instance refresh: exact-artifact admission

Implementation source: `9f4042f00cfd6227fb1418b92e7f3b1b5ce3e4c4`.
Normal Mobile/Diagnostic shaders and all70 assets unchanged; no investigation
counter remapping, extra query, CPU probe logging or player tuning. Discrete
instance-definition changes trigger BUILD; ordinary motion retains UPDATE.
Cache ownership follows successful submit. Detailed hypothesis/negative results
and next step: [single finite record](../../ENGINEERING_1_6_1_S24_INSTANCE_HITS_2026-10-01.md).

Android exact APK SHA256:
`a627c4a6431f40708e14327f9585bd0e6c0e7ca3133a9ded97242dfd7763d761`.
Both installed/pulled hashes match. `sourceDirty:true` records preserved unrelated
untracked raw receipts, not an uncommitted implementation. No app data cleared.

- [S24 two captures](s24-captures/capture-manifest.json), run20261002-010051:
  modelled hands restored, native player pixels66584/167535, honest compute RT,
  strict ASTC, Home/resume PASS. Lead inspected both PNGs. Corrected visibility
  is intentionally different from the missing-hands control, not a loosened gate.
- [S24 continuous route](s24-live-route/route-replay-state.json), run010411:
  13 waypoints, complete/not failed,1840 skin updates, maximum socket error10um,
  endpoint player visible/12975 primary pixels. Lifecycle not requested in this
  row. Owner live hands/enemy response remains pending; no per-frame visual proof.
- [S26 six captures](s26-captures/capture-manifest.json), run010625:
  pipeline RT, strict ASTC, Home/resume PASS. [All six PNG hashes identical](s26-within-backend-comparison.json)
  to normal8CB run20261001-222521 retained locally from the prior finite experiment.
- [RTX pipeline comparison](windows-pipeline/capture-comparison.json):13/13
  exact PNGs, median15.58345 ->15.53295ms; unchanged2% capture-timing gate PASS.
- [RTX compute comparison](windows-compute/capture-comparison.json):13/13
  exact PNGs, median16.18805 ->15.51560ms; unchanged timing gate PASS.

Windows frozen Debug executable SHA256:
`485d8743c8b1a91dfdc69eec2f835926824cac6fd88996b21e92d9eaa0d3e8f3`.
Same RTX5050 Laptop, Mobile/Diagnostic,960x540/100%, no portrait; unchanged
pipeline/compute controls compared separately. These timings are regression
capture evidence, not an optimisation claim. Existing backend-parity failures
remain distinct and open. No Shipping/S25/final-candidate acceptance inferred.

S24 GPU inventory is unchanged:43 buffers,53 allocations,17 BLAS,1 TLAS,
21 instances,2 pipelines,9814976 host-visible /72250176 device-local bytes.
Two21-entry CPU caches add2688 bytes plus flags, not a GPU intermediate buffer.
No sustained performance sweep was repeated. Keep the prior opacity NO-GO.

Raw full capability/process/thermal reports, executable/APK and build logs stay
local in `C:/Dev/tmp/horde-s24-instance-hits-20261001`; this archive retains the
bounded scene/state/validation/comparison receipts only. Current-head CI and
owner live S24 acceptance are the next unfinished admission checks. No release,
main merge or publication. Audio/haptic manual revalidation required:NO.
