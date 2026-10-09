# Optional indoor dust prototype, 8 October 2026

The owner expanded the active 1.6.2 scope with a bounded indoor-dust prototype.
This specifically supersedes older optional-dressing exclusions for this feature;
all other tomb/adventure/release boundaries remain. The supplied research brief
reviewed 62b6d40e; the actual clean starting checkout was ea5329bb on the existing
PR18 handoff branch. Existing guard, combat, mist and physical-device evidence is
preserved under its own exact identities. No phone work is authorized while the
owner is away carrying that device.

## Bounded core

`scene/atmosphere/IndoorDust.h` provides cosmetic, absolute-simulation-time world
positions, validated authored bounds/seed/drift/density and box/ellipsoid shapes.
The level data authors two small, inset tomb zones. Admission is at most two of
four authored zones, in stable-ID order. Low generates 16 motes per admitted zone;
Standard 32, with a global capacity64. These are experiment limits, not measured
mobile budgets. Conservative camera-projected 32x18 tile lists contain at most
four candidates each. Overflow is counted and rejects additional references;
there is no room-wide all-particles-per-pixel traversal. Movement does not reseed
or translate motes with the camera. Pause uses the same simulation timestamp.

Off is the fresh/saved migration default. The core Off branch skips authoring
validation, particle generation and tile projection, leaves upload bytes alone,
and reports zero owned work. Host tests pass for these properties, deterministic
motion, invalid data/capacity, overlap admission and camera independence.

## Integration and acceptance gates

The shared RT prototype is integrated for Pipeline and RayQueryCompute.
It uses primary depth, linear radiance composition and actual bounded existing
light visibility, without particle geometry/lights or a raster pass. Primary-only
transport is an explicit initial limitation: dust behind glass/water and reflected
dust are deferred. Keep authored zones away from the transmission/mirror proof
spaces; do not broaden those sightlines without secondary-view review.

The first unrolled shader probe exceeded the existing29 ray-query-site driver
safeguard (36). It is rejected and retained privately. A bounded candidate/source
loop probe meets that safeguard: Mobile opaque27, High opaque29, generic2 sites.
Measured changed-source static size is +31,612B opaque / +21,096B generic. Updated
ceilings use exact measured values, with no spare headroom; Shipping still has
zero atomics/no binding22. Static code growth is not dynamic cost acceptance or a
phone pipeline/presentation pass.

The core checkpoint9455c237 has all12 push/PR CI jobs passing in runs
37692764564 /37692771232. Changed-source Windows compilation and all4 Android
native ABI assembly pass. Ten affected native tests pass, including cache pause,
Off, projection, persistence, owning-completion, resources and fixture admission.
Android unit tests pass182/182 after the Debug inspection extension.
ABI/shader freshness and remaining exact integration checks are being completed.
Independent read-only source review found no consequential core/resource/shader
bug; it is not runtime visual validation. Desktop images/motion/cost and the new
phone comparison are still pending. Nothing here closes
owner appearance, Android motion, sustained thermal/performance or mobile
acceptance. Default Off remains authoritative until those decisions.

No shaft layer is implemented. Shafts stay deferred behind a clear motes visual
and cost go/no-go review, including mobile evidence. Existing ground Mist is
unchanged; any future continuous shaft would obey Mist Off. Dust motes are an
independently explained option and may be on with Mist Off.

Audio/haptic manual revalidation required: NO for this cosmetic slice; gameplay,
event timing and playback inputs are unchanged. Final 1.6.2 combat/audio/haptic,
quality-profile, owner and Eric audit gates retain their existing status.

The owner has now returned. Read-only device identification confirms the intended
SM-S948B / Android16 / API36. New device evidence must name the exact new APK;
none of the build/source results above is a phone dust presentation or cost pass.
The immutable renderer candidate will add requested/uploaded/completed Dust tier
and bounded CPU work to diagnostic stills and actual Dust tier to benchmark rows,
motion rows and consented summaries. Missing historical Dust evidence remains
absent, rather than being relabelled Off. Existing consent remains required.

Retained intermediate failures include the obsolete all-quality-bits Mist source
assertion, old frozen shader snapshot pins and stale compatibility includes.
Mist now gates only its own bit, with a negative mutation test rejecting use of
Dust bits. Compatibility artifacts and exact literal snapshot pins were refreshed
from compiled/validated current shared source. Hard driver/query/Shipping limits
remain. A manifest check correctly rejected concurrent source edits; it passes
when edits are stopped. The first Android Debug admission fixture caught mutually
inconsistent motion/checkpoint flags. Root review then corrected scale ordering:
Dust override must precede the existing explicit Debug scale request. These
corrections preserve settings; no phone test is implied by the host results.

All20 unique affected native/contract checks now pass: the10-test integrated
suite, eight unaffected-by-follow-up ABI/evidence/shader checks, the corrected
frozen manifest check, and the fully corrected artifact/compatibility check.
The consented Dust summary regression additionally passes after its metadata
change. Final Windows build and final4-ABI Android assembly pass from stable
runtime sources; Android Java182/182 remains current. A wrong per-command newline
setting produced a false CRLF whitespace report; ordinary repository settings
are checked before committing. No source content was changed to address that
invocation error. Exact runtime artifacts and visual evidence follow below when
sealed; the build checks still do not close sustained/device/owner gates.

## First immutable candidate and owner refinement

Runtime `71a2a98be945f9bb51ebb432f5ca20762a194eed`, tree
`b6b09a1aadc28f3a70fdefe119b797de7fa0f786`, is sealed separately.
Four-ABI Debug APK SHA-256
`8d01f27e30bfcb23994700409d4ec9071670352811c98a972994c6894a3b1a67`
(138,462,724 bytes); Windows Debug executable SHA-256
`a098d0c611192497a0e89f5122709de893776ab0261a35bbcbb1bcb15078d4db`
(11,549,184 bytes). Installed Debug base pullback matches the APK.
Source CI passes all12 aggregate jobs: push37696765708 / PR37696772234.

On the allocated SM-S948B / Android16 / API36, frozen Debug native RT stills
pass with12 current owning completed/presented frames: first box Off/Standard
on Pipeline, and second ellipsoid Standard on RayQueryCompute. Output is
1440x2980; actual traced50% is720x1490; native UI resolution is unchanged.
Mobile fire/water and the saved custom tuple are preserved. Each owned process
is stopped and confirmed preference entries remain unchanged. Private raw
thermal/memory/battery samples are retained; these short frozen inspections
do not establish moving appearance, displayed/sustained FPS or reliable power.

Both Windows backends pass Standard box/ellipsoid captures with owning frame,
source/executable/PNG joins and zero synchronization-validation error markers.
The first Pipeline Off launch exceeded its45-second external startup deadline;
its failure remains. A bounded120-second retry passes in7.35 seconds. This
external inspection deadline does not change native acknowledgements or the
normal15-second Keep/Restore confirmation. No cold-cache improvement is inferred.
Both backends fail the original wall view's required primary-arm visibility
(armPixels=0); rendered images and failure ledgers remain private. This is an
inspection-framing failure, not a settings/persistence failure or accepted pass.

The owner requests smaller/subtler motes after the first box phone image. The
follow-up reduces box/preview radius from12 to8mm, ellipsoid11 to7.5mm, and
opacity density by25% (.16 to.12; .14 to.105). Seed, drift, authored bounds,
counts, resolution, other effects and default Off stay unchanged. Developer
wall view159 moves to an oblique entrance view with a lower pitch; ordinary
primary-arm visibility guards remain mandatory. Affected native build and
core/development-fixture tests pass. The four-ABI Android Debug assembly and
182/182 Java tests also pass. Exact image validation is recorded separately
after sealing this follow-up. The first appearance is
not relabelled accepted.

## Smaller/subtler immutable follow-up

Runtime `998137c94448b28bec1da3f6bf74f875ea26004f`, tree
`19af308ce3439c942bad0296a45026cc318e501a`: Debug APK SHA-256
`41d9802db1f44f175e1a1cc8d33e73c8b3e1e39c9398e74ac5ca6370780fad09`
(138,462,724 bytes), installed base hash matches; Windows executable SHA-256
`3a6af703d83dd27620421f22e7769a0c685673b59f3c1e3bd7fd881ce83d3513`
(11,549,184 bytes). All four ABI payloads, closed asset admission, packaged
manifest and16KiB alignment pass. Source CI passes12/12 aggregate jobs in
push37698340306 / PR37698345979. Documentation after this seal has separate CI.

The owner accepts the smaller/softer first-box phone still from this exact APK
(Pipeline Standard PNG SHA-256
`f606e4df362466958ecfeea2a6b84164085f54a1b5f494a7f2cf20b4a47a57dd`).
This is appearance approval for that still, not moving/thermal/performance
acceptance. Its matched Off PNG is
`a54b9de6972f5e1e34a882738bab86ee0e15f34e548ae1de0c7e1970a5446dac`.
The phone also passes refined ellipsoid Standard and first-box Low frozen
captures on RayQueryCompute, each with12 current owning completed/presented
frames. Native UI remains at output resolution; traced50% is720x1490 at
1440x2980 output. All automated sessions preserve preferences and stop their
owned Debug PID. Production/benchmark packages are untouched.

Both Windows backends now pass all three Standard box/ellipsoid/wall captures
with exact shader, completed/presented submission and PNG joins, and zero
synchronization-validation error markers. The reframed wall retains required
primary-arm pixels; the previous159 failures remain recorded above. No guard
was weakened and no gameplay camera changed.

Real phone `torch-low-opening` movement passes the existing60Hz harness on
Pipeline Off/Standard (565/564 state and completed-frame rows;16 captures each)
and RayQueryCompute Standard (619 rows;16 captures). Every completed row has
the exact requested Dust tier, current scene/surface/measurement identity and
owning submission/completion; all capture hashes and state/frame joins pass.
Both backend Standard runs have zero recorded fixed-step overruns. These are
harness-generated input schedules, with timed native captures/readbacks, not
owner touch latency, continuous-video acceptance or sustained/scanout evidence.
Sampled views cover slow turn/translation, the opening/walls, dark masonry,
changing torch pose/light and attack/parry. Continuous tiny-particle stability,
second-zone moving appearance and relevant glass/water/mirror inspection remain
open; primary-only transport limits are unchanged.

One ordered short Pipeline pair (Off first), same APK/backend/output/route and
saved Mobile water/fire, Glass On, Current shadows, cap30, Mist On, supplies
these existing timing fields. Approximate24.4-second runs include readbacks.
They are not steady-state shipping benchmarks and do not isolate thermal drift.

| Stage / metric (ms) | Off median / p95 | Standard median / p95 |
| --- | --- | --- |
| Approach GPU RT command buffer |38.627 /40.954|38.715 /41.307|
| Approach whole-frame CPU |48.445 /54.613|49.455 /54.726|
| Torch motion GPU RT command buffer |27.974 /32.520|28.040 /32.567|
| Torch motion whole-frame CPU |40.261 /46.441|40.171 /46.245|

No reciprocal GPU timing, row-count rate or median is called displayed/sustained
FPS. This pair neither proves zero dust overhead nor practical sustained30FPS.
Observed tracked host-visible/device-local allocation bytes are unchanged
(11,164,928 /85,223,296); these are renderer-owned allocations, not total app
PSS, driver memory or a reliable power measurement. Private short-run OS
thermal/battery/memory snapshots remain insufficient for sustained acceptance.

Recommendation: keep the owner-approved motes as an optional default-Off
prototype; defer shafts. Required next gates are sustained matched Off/motes
cost, reliable memory/power/thermal evidence and moving/secondary-view review,
without lowering other quality. Haze-only/combined comparisons are not applicable
because no shaft haze is implemented; they remain deferred, not passed.
The broader integrated50/40/33 profile decision, final owner audio/haptic review,
production encounter activation and Eric's independent audit remain separate.
