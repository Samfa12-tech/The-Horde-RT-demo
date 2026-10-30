# Player template copy: one desktop candidate

The world-player upload now receives its immutable template once after every
successful validated asset load. Each solved pose still writes all four
position, normal and tangent components, preserving the complete 16-byte UV
field. The full GPU upload, ABI, cadence, simulation, shaders, BLAS/TLAS and
viewmodel are unchanged. One 34,304 × 64 = **2,195,456-byte representation copy**
is removed per refresh; this is not measured memory-bus traffic or an allocation
saving. The same steady upload allocation happens earlier during asset loading;
its earlier startup peak contribution has not been separately measured.

Load entry clears the cache before any failure/preview exit; successful layout
validation always initializes it, including same-size reloads. The only other
production template mutations are move and destruction, which retain paired
ownership. Static scene replacement preserves the dedicated player streams.
No per-frame hash, worker, pre-fence change or texture-LOD change is introduced.

## Bounded measurement

MSVC x64 Release, Intel Family6 Model186 Stepping2, Windows Balanced. One
opening-control/candidate/closing-control batch, with no concurrent owned build
or renderer. Each result contains301 batch averages of16 actual packing calls
after64 warm calls. The already solved actual player pose is reused; skinning,
Vulkan upload and GPU execution are outside the timer. The null-observer CPU
helper is the same production seam in both builds. Control retains the old
per-refresh full assignment after the same new load initialization/size guard.

| CPU packing component | Opening control | Candidate | Closing control |
| --- | ---: | ---: | ---: |
| Median, µs per call | 266.113 | 117.013 | 264.337 |
| p95 batch average, µs per call | 425.769 | 198.037 | 357.981 |

Control medians drift0.667%; candidate median is55.7–56.0% lower, approximately
**0.148 ms** below the mean control medians. All raw batch averages and exact
binary/source hashes are in [receipt.json](receipt.json). These hot repeated-pose
component measurements do not establish full-frame/FPS, individual-call tails,
sustained behavior or Android benefit. No CPU temperature or per-core scheduling
trace was measured. The proposed0.15ms phone criterion has **not** been tested.
There is no further tuning iteration in this checkpoint.

## Correctness and build evidence

- Full64-byte record parity against the legacy algorithm over12 actual solved
  forward/reverse-phase/crouch poses. Short uploads reject; same-count reload
  replaces poisoned old UV fields; populated-destination move, self-move,
  device-owned destruction/reload and failed reload pass.
- Three affected Release CTests pass: resource inventory, static cutover and
  actual skinned-character smoke. Full Debug resource inventory passes24.63s.
  [Release log](release-tests.txt), [Debug log](debug-tests.txt). No gate changed.
- Paired hidden Windows RTX5050 native RT lookout captures are byte-identical:
  PNG SHA256 `a303359e63eeebedd9cf4b77c3a2ef26a8fd3ea32b34f4ef714fb15328ef79c3`.
  Both honestly present960×540 RT-produced frames with identical Diagnostic/High
  shader hashes. The image was inspected; foreground and owner settings remain
  unchanged. These frozen frames validate output, not copy performance.
- Android arm64 Debug/O2 Shipping/Mobile route-probe build passes. APK SHA256
  `dcab8e2e6cc2b576a6758e660816b0591ecf77bc918eb692b6cbcdccd6e9d463`,184,175,246
  bytes. All199 decoded ZIP entries match preserved current-cache control
  c1d65749 except `lib/arm64-v8a/libhorde_rt_probe_android.so`; no asset/shader
  entry changes. Candidate is archived locally and **not installed**.

The fixture's initial missing namespace and CPU-only Destroy assumption, first
unquoted Gradle invocation and wrong expected native filename are retained as
setup failures. The destruction test now uses this existing fixture's stubbed
device-owned branch; production Destroy was not changed. Failed preflight
benchmark outputs are not admitted. Raw files, binaries and images remain in
ignored `reports/player-template-copy-20261023/`; large artifacts are not committed.

The benchmark can be run through `horde_rt_scene_resource_inventory_tests.exe
--player-upload-copy-benchmark`; `--player-upload-copy-check` selects only the
focused correctness fixture. Normal CTest still runs all prior inventory gates.
For the semantic control, restore the template assignment immediately before
the dynamic-field loop in `PackPlayerWorldBodyPoseUpload`; keep all other seams
and build settings fixed. The separate benchmark CLI branch has an identical
caller in both measured binaries.

Retain as a **desktop-validated source candidate, Android acceptance pending**.
The phone has been restored/released; no further phone operation is planned.
The [current phone control](../2026-10-23-current-cache-phone/README.md) and its
Android Perfetto config-read denial remain separate. Next resolve a permitted
trace handoff and reviewed production-bank GPU attribution, then test these
archived exact APKs in a new matched, owner-offered window. Do not stack a worker
or frame-overlap experiment here. PR conflicts, hosted MSVC cap and Debug route
deadline remain open; no merge/release.

Audio/haptic manual revalidation required: **NO**; gameplay/event inputs and
playback are unchanged.
