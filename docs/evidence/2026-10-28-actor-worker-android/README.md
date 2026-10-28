# Android actor worker: same-APK experiment preparation

This continues the existing exclusive Kit CPU worker; no new animation system,
pre-fence work, GPU buffer ownership, shader bank, frames-in-flight, quality,
render scale, affinity, priority or gameplay speed changes. Normal builds remain
OFF. No phone/ADB, installation, native game launch, CUA or foreground input was
used. Phone is reserved to Reef until a new explicit handover.

## Bounded change and ownership

- Android now explicitly compiles the existing worker only with
  `hordeActorWorkerExperiment=true`, Debug checkpoint-enabled production
  Shipping/Mobile chapter route timing. Shader/asset/scale experiments are rejected.
  Release forces OFF. Experiment configuration skips production signing inputs;
  it uses the ordinary development-signed Debug build, not release signing.
- Package `com.samfa12.hordelanternrt.debug.actorworker` isolates owner apps,
  saves/settings and the frozen recording candidate. It defaults to serial.
  Strict `horde.debug.actorWorker=serial|worker` intents require both a debuggable
  app and the generated experiment-build flag. Invalid/missing/wrong-type values
  do not change mode; there is no public settings/control addition.
- JNI writes one coherent atomic mode/request epoch only. The renderer reads it
  once into an immutable frame after the existing graphics fence. The previous
  frame's job joined before return. Serial selection retires the CPU worker before
  actor access; worker selection dispatches before independent player work and
  joins before vertices/upload/refit. All earlier-return guards and retirement
  paths remain. No simulation or Vulkan call moves onto the worker.
- Each timed owner logs one `HORDE_ACTOR_CPU_PREPARED ... mode=serial|worker
  cpu_only=1` per request after an actual successful actor pose. A request alone
  does not prove execution. Cached/no-actor frames do not grant the witness.
  This is CPU preparation evidence, not GPU completion/presentation, scheduler
  utilization, per-core profiling, clocks or a frame-time gain.
- Native declared-input provenance now includes the compiled worker option.
  APK hash and embedded source stamps remain separate from that scoped digest.

## Local evidence and remaining acceptance

The experiment APK compiled all four ABIs and passed49 Java/widget tests (2 new
strict intent cases plus47 existing campaign/menu cases), with no failures,
errors or skips; Debug lint passed. Initial full experiment run took5m43s; the
final Gradle isolation/provenance refresh passed in2m16s. Current package metadata
retains development version1.6.2-debug/code10; this is 1.7 branch work, not a new
release version or production package.

Three existing Release CTests passed in9.45s: actor-worker parity and scene
initialization preflight/resource inventory. The parity fixture adds actual
serial/worker retirement/recreation and the production six-clip Kit asset, both
turn directions, four partial angles and five phases. Vertices and grounded
bounds remain byte-identical. It does not prove phone scheduling, visual joins
or a GPU resource lifecycle. Earlier idle/walk and owned-request/failure/stop tests
remain. The first host command named a nonexistent second target after compiling
the worker; corrected registered preflight/resource targets built and passed.
The original failed command receipt remains in ignored reports.

Short actual Gradle configuration cases check normal OFF, malformed property,
missing route prerequisites and explicit ON. Exact post-commit APK/stamps/ABI/hash
and latest CI belong to `reports/actor-runtime-device-ready.json` once prepared;
pre-commit artifacts are not presented as clean exact-commit builds. Source/host
proof does not enable normal gameplay or close any flask/ring/forest gate.

## First allocated-phone plan

Request a20-minute bounded slot once exact APK is ready. Verify actual connection,
Reef release, model/custody, installed identities and owner activity before any
operation. Install only the isolated test package when that action is authorized;
never uninstall/clear/overwrite owner apps or progress. No new security/tracing
access. Stop at any denial. Preserve original settings/rotation and close the
owned experiment at release.

Use existing chapter motion/timing capture, one fixed-resolution Showcase scene
and the same APK/shader/quality state in both modes. Record independent source/APK
identity and actual CPU-mode witnesses, actual submitted dimensions, presentation
mode/pacing, coherent phase/camera/Kit activity and thermal/power context. The
software30Hz preview sleep is limited to paused Entry/GraphicsPreview in the
current render loop; Showcase traversal bypasses it. That is not proof that the
platform/display cannot pace frames. Check actual intervals before interpreting
performance. No silent cap/quality/resolution change.

After equal warmup, A=serial/B=worker, bounded ABBA or opening/control/closing
comparison. Exclude worker construction from steady-state samples only through
identical predeclared warmup, retain dispatch/join and teardown costs separately.
Compare equivalent scene phases/populations; use the existing whole skin/frame
critical path, not worker duration or a sum of overlapping medians.3% is a useful
experiment gain, not a release target. Natural temperature/clock mismatch makes
causal gain inconclusive; retain warm field data and do not loop indefinitely.
No performance claim comes from these host tests or successful compilation.

Pre-fence preparation remains a subsequent audit/experiment, not part of this
patch. Vertex/palette staging, last prepared/uploaded/submitted identities, BLAS,
scratch/TLAS/instance buffers, failed submit/recreation and input age need proof
before overlap with the prior GPU frame. Do not just raise frames in flight.
