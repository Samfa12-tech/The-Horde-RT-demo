# 1.6.1 engineering and release reconciliation

October3,2026. This report reconciles the original audit and subsequent explicit
owner decisions; it does not reopen completed investigation. Exact publication
source is `a397757249871b6b64fe5b77fc14f24e8cfcbb2b`, runtime/resource checkpoint
`3d26ad6`, final music/source delta `ec13876`. Receipt/guard/docs commits leave
the frozen game binaries and assets unchanged.

## Programme disposition

| Finding / requirement | Disposition and authoritative evidence |
| --- | --- |
| F01 baseline recovery | **Resolved.** Recovered current-source/integration CI; fresh six-lane push/PR success at945f990 before publication. Original red results and bounded fixes retained, not relabelled a single97/97 Windows run. [Integration matrix](evidence/2026-10-02-final-integration/README.md). |
| F02 version/release identity | **Resolved for1.6.1; historical recommendation deliberately superseded.** VERSION/map1.6.1/code9, stable certificate, frozen provenance, public itch/GitHub assets and downloaded bytes match. The requested retroactive1.6.0 tag/release is superseded by the verified current1.6.1 announcement; no historical release was invented. Published-line guards reject package/sign/upload before side effects. [Release receipt](SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md). |
| F03 modelled first-person arms | **Resolved and owner accepted.** Independent native-RT Viewmodel/WorldBody buffers/BLAS, shared gameplay animation/IK/grips, named semantics, accepted live cuff/armpit/look-down/mirror. No normal block-arm/full-body-primary/overlay substitute. [Cuff](evidence/2026-10-02-right-cuff/README.md), [seam/owner evidence](evidence/2026-09-27-segmented-seams/phone/README.md). |
| F04 player semantic admission | **Resolved.** Processor, generated runtime, manifest, loader, atlas and actual static/skinned/reordered/malformed fixtures agree. Vulkan CPU-host lane runs affected contracts; it is not hardware acceptance. [Finding index](ENGINEERING_1_6_1_FINDING_STATUS.md). |
| F05 Shipping diagnostic overhead | **Resolved.** Eight actual extracted final Windows/Android Shipping modules validate/disassemble with zero diagnostic atomics and no readback binding22. [Freeze](evidence/2026-10-03-signed-freeze/README.md). |
| F06 performance observability | **Resolved.** Submission/fence/build/workload identity, complete row denominators and finite matched harness retained. Live benchmark counter implemented; its observer overhead remains unmeasured and it stays off during performance trials. [Windows counter](evidence/2026-10-03-windows-live-closeout/README.md). |
| F07 fixed quality variants | **Resolved.** Mobile/High and Shipping/Diagnostic pairs cover Pipeline and genuine BLAS/TLAS hardware RayQueryCompute. Quality is independent of backend/model name. [Backend record](ENGINEERING_1_6_1_RAYQUERY_BACKEND_2026-09-13.md). |
| F08 memory/resource ownership | **Resolved bounded work; broader staging deliberately deferred.** Persistent coherent dynamic mapping and compatible local/coherent immutable placement preserve failure/fence ownership. Heavy A/B gives no meaningful speedup; no universal device-local/OOM fallback claim. [Resource matrix](evidence/2026-10-03-final-s26-resources/README.md). |
| F09 frame pacing/concurrency | **Deliberately superseded.** Retain one frame in flight and correct host-written TLAS ownership. Owner accepts measured1.6.1 performance; no30FPS claim. Full graphics options planned1.6.2, multi-frame/temporal work not automatically promoted. |
| F10 stronger cross-platform CI | **Resolved.** GCC/Clang/MSVC portable, actual Vulkan CPU-host player/resource fixtures, selected Clang ASan/UBSan and Android four-ABI/Java/lint. Main protection requires all six checks/up-to-date PRs, prevents force-push/deletion and retains explicit admin bypass. [Read-back receipt](evidence/2026-10-03-release-publication/main-protection.json). Hardware/shader/device evidence remains separate. |
| F11 architecture extraction | **Resolved bounded seams.** Pipeline variants/bundles, GPU/resource lifetime, named player roles, evidence ownership, reporting and reusable Core PCM seams; no gratuitous renderer rewrite. |
| F12 contract/regression coverage | **Resolved.** Semantic/IK/topology/seams, physical math, resource failure, input/event timing, version, PCM/focus and reporting consent/retry/cancel tests supplement native/owner evidence. |
| F13 documentation | **Resolved.** Released README/platform guide/decisions/plan/finding/receipt indexes name actual ownership and publication; dated reports retain their original hashes and limitations. Complete commit inventory accompanies publication evidence. |
| F14 driver-version decoding | **Resolved.** Raw implementation-defined driver version, guarded identity/name/info/conformance and independent Vulkan API version; focused tests/RTX query/CI retained. |
| F15 Hotstrike redistribution | **Still open, owner controlled.** Owner issue/contact explicitly nonblocking publication. No grant, skeleton replacement, history rewrite or distribution workaround. |
| Physical reward-lantern glass | **Partly resolved; remaining defects deliberately superseded for1.6.1.** Accepted near-corner exits3b92329 and duplicate transparent candidates632322d retained. High complete dielectric path remains; remaining contact/near-edge/parity defects are [future investigation](../FUTURE_WORK.md#future-glass-investigation--owner-deferral-2026-10-03), not fixed or tolerance-relaxed. Mobile removes actual pane geometry by profile, no approximation. |
| Music and audio controls | **Resolved and owner accepted.** Canonical PCS JSON/PCS1, retained v68 app voices/live FX, A–H cue logic/bodies/tails, shared Pocket Audio Core PCM ownership, persisted independent music/SFX volume, quieter footsteps, Windows focus/startup fix. Only A/D Melody3 held drone removed; packaged listening accepted both. [Scoped delta](evidence/2026-10-03-music-drone/README.md). |
| In-game reporting | **Resolved and owner accepted.** Approved Briarhold-derived Cloudflare email with bounded schema/typed context, explicit consent, optional game-only RT screenshot, preview, retry/cancellation and offline fallback. Actual test email received; Windows prepare/preview/cancel accepted. No secrets in client. [Reporting record](ENGINEERING_1_6_1_REPORTING_2026-10-01.md). |
| Lifecycle/output resize | **Resolved bounded path.** Extent-only resize retains compatible assets/AS/pipelines and rollback; Android off-UI scene ownership and cancellation/resume repair. Current-bank S26 motion/scale/resume and final signed update smoke pass; readiness remains slow, not instant. [Interaction](evidence/2026-10-03-final-s26-interactive/README.md). |

The original secondary-only world-body recommendation is deliberately superseded
only for the owner-required look-down body presence: explicit remainder primitives
receive primary visibility, excluding world arms/head; independent viewmodel arms
retain their own geometry/resources. This is not full-body primary visibility or
a pitch-specific overlay. `PlayerRenderSlot.cpp` owns the named mask contract.
Accepted normal-pitch live/owner evidence remains the boundary; visible feet and
steeper pitch are future work.

Broader device-selection scoring/enumeration hardening and a general structured
diagnostics extraction from audit section7.4/11 remain low-priority future work,
not claimed implemented by the driver-version fix. The owner's explicit measured
performance acceptance supersedes the conditional1ms skinning target for this
release; no per-viewmodel1ms measurement or sustained30FPS result is invented.

## Test/platform matrix

Publication checkpoint945f990: push37089806774 and PR37089810005 both completed
success, all six lanes. Post-publication docs/guard closeout and normal main
integration have separate current-head checks:271f6c0 push37091774573/PR37091777614
both six-lane green; [PR15](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/15)
merged normally at ee6d877 with identical tree and green main37092128244.
Receipt-only main006c288 also passes all six lanes in
[run37092782420](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37092782420).
Final documentation changes do not change the frozen runtime; their own current
head checks remain visible in main Actions rather than causing a receipt loop.

| Scope | Result / limit |
| --- | --- |
| GCC / Clang portable | CI success; inspected rosters59/59 each. No hardware RT. |
| MSVC portable | CI success; inspected roster65/65, not all Windows tooling. |
| Vulkan CPU-host |17/17 selected actual player/skinning/semantic/resource fixtures. No device creation or presentation. |
| Clang ASan/UBSan |15 explicitly selected fixtures pass, not full-driver/race certification. |
| Windows broader integration | Original failed rows preserved and relevant repaired rows pass; separate Debug captures, Release build/tooling and actual Pipeline/Compute live receipts. No invented single97/97 pass. |
| Windows RTX5050 | Exact retained executable/current assets: Pipeline presentation, required Compute route1838/1838 valid presented CPU/GPU rows, counter/cancel/restart/completion and owner cuff/audio acceptance. Archive metadata-only, not a new performance/parity run. |
| Android build/package | Four ABIs, Java/lint, production1.6.1/code9, strict asset bank, established certificate, APK plus all12 ELF LOAD segments16KiB aligned, actual packaged Shipping SPIR-V pass. |
| SM-S948B / S26 | Final signed update over observed1.6.0 without clearing data, exact pulled-back APK/settings, strict ASTC/honest RT, modelled hands/two enemies and same-PID Home/resume pass. Saved76% retained; not a fresh75% performance matrix. |
| SM-S928B / S24 | **Owner/device validation required in future:** working development imagery, not fully tested final release. Owner explicitly defers final matrix; no further1.6.1 phone action requested. |
| S25 | **Owner/device validation required:** exact device unverified; S26/RTX/S24 do not certify it. |
| Update announcement | Actual public metadata passes unchanged1.6.0 parser selection1.6.1; current1.6.1 is up to date. Old-install platform popup/network flow not retested. |
| Release guards | Current focused Release version/policy CTest3/3 pass. Real package/sign/upload entry points reject1.6.1 before side effects. Canonical post-publication preflight passes tag/release present-matched, no fixtures/skips. |
| Audio/haptic manual revalidation required | **NO** for closeout: docs/receipts/guards only; accepted runtime, event timing, PCM/gains/assets unchanged. |

## Performance conclusions — preserve negative evidence

The finite physical/open-profile matrix contains24 completed reports at75%,
within each device's own extent/backend. [Full table/thermal context](ENGINEERING_1_6_1_MOBILE_LANTERN_PROFILE_2026-10-01.md):

| Native render-entry-through-present median | Physical controls C1/C2 | Open Mobile candidates P1/P2 |
| --- | --- | --- |
| S26 held-high |235.87 /272.80ms |112.03 /114.23ms |
| S26 live reveal |236.44 /278.82ms |114.88 /122.50ms |
| S24 held-high |265.33 /346.32ms |84.17 /84.47ms |
| S24 live reveal |279.71 /354.88ms |100.45 /109.65ms |

These are whole quality-profile treatment measurements, including automatic
OpaqueFast selection, not isolated pane throughput or actual display FPS.
Controls differ thermally/order-wise; do not pool devices or claim a universal
speedup. Ordinary S26 opening remains about71–76ms GPU in the candidate cohorts.
First-confirmed-blocker and other rejected omissions remain negative evidence;
the staged-primary prototype failed unchanged image equivalence and was not
promoted. No unsupported low-risk optimisation was forced into Shipping.

Later open-profile resource held-high pair93.6585 vs93.6380ms is effectively
unchanged. Candidate route67.8279ms and standalone warm reveal121.2535ms are not
matched gains or sustained30FPS. The estimated22–24ms GPU allowance for30FPS is
still far below observed heavy GPU workloads. CPU skinning is independently
material but cannot explain away the GPU gap. RAM/Graphics snapshots are not
hardware bandwidth/cache/stall/occupancy/register/spill counters; unavailable
counters remain gaps. Owner locks performance as-is for1.6.1 and plans graphics
options1.6.2, rather than another broad optimisation/quality-reduction pass.

## Distribution and next release

- [itch](https://samfa12.itch.io/the-horde): Windows2055201 / Android2055202 ready,
  version1.6.1; downloaded payload admission passes.
- [GitHub v1.6.1](https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.6.1):
  public non-draft prerelease; exact package sourcea397757 and all three assets.
- [Publication receipts/commit inventory](evidence/2026-10-03-release-publication/README.md)
  and [release validation](SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md)
  bind hashes, source, tests and device scopes. Frozen packages must not be rebuilt.
- Normal main integration and recoverable local cleanup are [verified](evidence/2026-10-03-release-publication/closeout.md),
  preserving unrelated future1.6.2 intake, scratch and historical dirty
  experiments. A clean release is not permission to delete unknown work.
- Recommended next line: owner-planned1.6.2 with complete graphics options,
  new version/code greater than9 and scoped future investigations. No automatic
  temporal renderer expansion, S25 certification or licence remediation.

No further physical-phone checks are required for the owner-accepted1.6.1 scope.
Future S24 final-candidate/S25 testing and High-glass investigation stay explicitly
open under owner deferral rather than being silently declared green.
