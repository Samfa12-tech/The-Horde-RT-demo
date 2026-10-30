# c2f39ece: confirmed job cap, incomplete hosted acceptance

Exact [run 39948002864](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39948002864),
MSVC job `120395300969`, is terminal. Its check annotation explicitly says:
**“The job has exceeded the maximum execution time of 15m0s.”** This was a time-cap
cancellation, not a superseded run. The secondary cancellation annotation and
[sanitized exact-head receipt](c2f-terminal-receipt.json) are retained. No manual
rerun, cancellation, test exclusion or workflow change occurred.

GCC, Clang, Android, sanitizer and Vulkan-host jobs pass. MSVC builds successfully,
then passes 104 of its unchanged 149 selected tests before cancellation during
test 105, `horde_rt_windows_glass_graphics_contract_tests`. That test produces no
result. No assertion failure or test hang is established. Native report UI, the
remaining tests and the later APK resolver do not complete in this run; do not
borrow their prior passes to label this head green.

## Phase evidence and headroom

| MSVC phase | Passing 749e5ffe | c2f39ece |
| --- | ---: | ---: |
| Configure | 23 s | 72 s |
| Build | 465 s | 653 s |
| Test selection | 122 s, all 149 | 130 s, cancelled after 104 |
| Four provider-object evidence wrappers | 74.41 s | 92.84 s, all four pass |
| Whole job | 653 s | 909 s, cap cancellation |

The earlier 10m53 pass is real; it does not establish robust headroom. Build plus
configure now consume 725 seconds, leaving most of the sequential test work close
to the cap after checkout/setup. Those phases grew by 237 seconds. The same first
104 tests took 101.72 seconds in 749's per-test sums; its remaining 45 totalled
14.04 seconds. These are comparisons across different heads/runners, not a
controlled CPU measurement or a proof that all variance is environmental.

The requested KitSourceIntake compile/link interval was unavailable in both
inspected MSVC logs. Follow-up source inspection explains that absence: this lane
configures `HORDE_RT_BUILD_VULKAN_TARGETS=OFF`; `horde_rt_probe_core` is created
only when that flag is on, and both changed Kit/worker test targets are guarded
by `BUILD_TESTING AND TARGET horde_rt_probe_core` (CMake lines 358 and 1696).
Those two files are not compiled by this MSVC lane. Their direct compilation
therefore cannot explain its longer build. The broader cause is still unresolved.
The four provider wrappers alone rose 24.8%, while configure/build also grew;
no single newly hanging test explains the observed cap result.

## Bounded next choices; no change applied

There are two concrete duplication seams worth checking without dropping coverage:

1. `CMakeLists.txt` directly compiles the same Shipping/Mobile provider into
   `horde_rt_pipeline_variant_provider_tests` and
   `horde_rt_pipeline_bundle_contracts_tests`, while the four variant fixtures
   already build a dedicated `horde_rtpv_0_0_obj` for those same 0/0 definitions.
   A small test-build reuse could remove those two duplicate compiles if the
   complete compile definitions/options, generated dependencies and link outputs
   are equivalent. Retain all four distinct variant objects, native fixture
   executables, containment checks and negative controls. No measured saving yet.
2. `RtPipelineVariantFixtureEvidence.ps1` decodes all six forbidden include streams
   again for the contaminated-word-stream negative after the positive check.
   The test mutates a provider-byte copy, not the includes. A process-local cache
   could bound this parsing at eight distinct source streams instead of up to
   fourteen calls per fixture, without sharing compiler objects across variants or caching
   a containment verdict. Scanner work still runs on each independent adversarial
   input. Its timing benefit is unmeasured; this is a follow-up option, not a win.

Do not stack those changes or repeat whole CI runs as experiments. If pursued,
start with one seam, a focused local control/candidate measurement and the existing
positive/negative fixture gates, followed by one normal exact-head checkpoint.
Changing the scanner or compiler artifact boundary requires particular care;
keeping a fast but weaker predicate is not acceptable.

An **explicit MSVC-only 20-minute whole-job cap is also justified to consider**:
the current 15-minute envelope can expire after a valid build while valuable
tests are advancing. It would allow the observed remaining work plus headroom,
without increasing any individual CTest deadline or reducing coverage. It does
not make ordinary passing runs last longer; a stuck job could occupy up to five
extra runner minutes. Twenty minutes is a bounded proposal, not a proven worst-case
bound or an applied budget change. Linux's cap and all existing native/test gates
should remain unchanged. No runner tier, paid service, permission or security
change is proposed.

Recommendation: preserve this failure, take one small provider-build reuse check
if desired, and make the Windows job-envelope decision explicitly. Do not infer
robustness from another lucky pass or relax gameplay/Debug route deadlines.
