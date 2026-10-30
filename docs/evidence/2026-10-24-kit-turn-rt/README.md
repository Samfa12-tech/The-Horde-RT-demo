# Kit step-turn: isolated RT joins, 24 October 2026

**Test-only playback proof; production turning is unchanged and unadmitted.**
The cleanup/native source at `9928d23b` now reaches the actual Windows hardware
RT renderer through a small Debug-only extension to the existing hidden lookout
fixture. Original clips, geometry, gameplay snapshots, route motor and saves are
unchanged. This is not the shared gameplay turn-state implementation.

## What was exercised

The separate inspection actor requires the candidate's exact original index/UV,
primitive and material addressing before reusing the existing Kit upload/BLAS
lane. Its four explicit test slots are Idle, Walk, left turn and right turn;
production Attack/Dead or default loading are not rebound. Both CPU preparation
paths include inspection identity in pose invalidation. A source switch retires
the exclusive worker; reload/move/destruction retire it before its actor.

The fixed-step diagnostic sequence renders both 45-degree / 0.75-second takes:
nonzero Idle phase -> existing 0.18-second blend envelope -> authored turn ->
existing Walk with the real route fixture's supported roots/distance phase.
The declared external model-yaw curve is applied once, with its negative logical
yaw convention. Both endpoints face the same route bearing, 2.00320 radians.
The earlier [native coordinate proof](../2026-10-23-kit-turn-native-export/README.md)
remains the numerical single-yaw/negative-control evidence.

Every retained candidate frame is joined to a completed submission with actual
RT dispatch, swapchain copy and successful presentation. Selected phase is checked
after completion against the uploaded pose key, rather than inferred from a
filename. Mid-turn clocks freeze for two renders, advance once on resume, then
clear back to the ordinary actor. Original-image equality is checked on each
reset and before candidate data release. Existing Resume/menu/scene-retirement
checks then run with all diagnostic overrides cleared.

Pause/resume here concerns the **explicit inspection playback clock**, while the
isolated loaded campaign remains paused. It does not prove interruption of a
future gameplay turn state. No physical input, foreground activation, combat,
earned journey, phone operation or owner feel acceptance is claimed.

## Actual image review

[Left sampled RT sequence](left-rt-contact-sheet.png) and
[right sampled RT sequence](right-rt-contact-sheet.png) show unmodified crops at
native pixel size from the real captures, with stage labels. The retained
[full frame](left-turn-full.png) establishes their camera/lighting context.
The sheets are sampled sequences, not continuous video or generated stand-ins.

Both directions show the feet exchanging and the body reaching the intended
heading. There is no obvious knee inversion, large vertical snap or pose pop in
these sampled joins; the end-turn and zero-weight Walk entry match. The return to
the ordinary image is exact. Boots and shadows remain close to the ground in
the available view. Night lighting, grain and sampled intervals limit fine sole
sliding judgments; this does not replace the retained offline 6.856 mm contact
measurement or establish a new contact tolerance.

The current route still begins translating immediately at its existing cruise
speed. This trial intentionally leaves that motor intact. It therefore does not
admit natural acceleration or solve the original wrong-facing initial departure
in production. A short supported turn/start, opposite or interrupted turns,
arbitrary angles/phases, slopes and speech overlays remain integration work.

## Checks, custody and limits

On the RTX 5050 Laptop GPU (NVIDIA 610.47), each serial/worker run passed
**77 checks / 49 RT frames**, retaining 38 turn/reset images in about 19.55 and
18.47 wall seconds respectively. These durations include loading/capture and are
not performance comparisons. All 38 PNG hashes match between modes. Per-frame
internal/output dimensions are 1232 x 803 with DRS off and unchanged High settings;
resource counts/bytes stay equal across the inspection stages. Three focused
actor intake/locomotion/resource CTests pass, as do the optional-worker retirement
check and Release renderer-core compilation. Default-OFF restoration is recorded
with its final executable identity in the receipt.

Exact executable/source/asset/image identities, per-stage completion identities,
dimensions, resource inventories, original failure references and CI are recorded
in [receipt.json](receipt.json). Full local logs and all captures remain under
`reports/kit-turn-rt-validation-20261024*` and the named isolated fixture folders.
Only the two sheets and one contextual frame are backed up, avoiding a raw-frame
dump. The initial build's image-field typo is retained in its original failed
log; the corrected build and actual run follow it.

The fixture remains hidden/noactivate and keeps its original 90-second / 180-frame
limits and save-write rejection. Owner campaign/settings, the original source
and runtime Kit, the candidate files and all 151 parked owner files are preserved.
No extra GPU actor allocation is introduced; the separate CPU asset's loading/RAM
cost is not measured or proposed for production. No performance gain is claimed.
The opt-in worker remains OFF after validation. Android/Windows packages still
select the original runtime asset, excluding these source-only proof files.

Build the existing Debug diagnostic target and invoke only the bounded fixture:

```text
HordeLanternRT.exe --lookout-validate-hidden --lookout-inspect-kit-turns --campaign-test-root <fresh-isolated-exported-ForestEndpoint-folder>
```

The existing guards require that fresh caller-created directory directly beneath
this checkout's `reports`, named `horde-campaign-fixtures-*`, containing only its
canonical manifest and test autosave. Owner records are not an input. The adapter
loads the fixed owned source candidate, not a caller-selected arbitrary asset.

Next: a separately scoped shared-simulation turn/start state using these clips,
with transient phase, supported partial/opposite-turn policy and gentle departure
tested against dialogue/path/lifecycle contracts. Do not enable production
bindings on the strength of this diagnostic proof alone. Benchmark/torch and
deferred forest work remain untouched. Audio/haptic manual revalidation required:
**NO**, because production animation/event timing and playback are unchanged.
