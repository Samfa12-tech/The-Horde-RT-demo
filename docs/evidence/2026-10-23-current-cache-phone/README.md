# Current normal-cache phone control, 23 October 2026

One valid 40-second stationary lookout hold on SM-S948B (Android17/API37),
using runtime bceed0c1 and exact APK c1d65749 (full identities in
[receipt.json](receipt.json)). This is the current normal-cache build, not the
earlier reverted-cache control. No player-template-copy candidate was present.
RT Pipeline, Shipping/Mobile,33%,475x984 dispatch; native view2981x1440,
rotated swapchain1440x2981. Water1,Fire0,cap30,GlassOff,Current shadows,
MistOff,Dust1. The UI remains at display resolution.

The preserved strict analyzer admitted620 completed owning GPU/CPU frames after
10seconds warmup, spanning29.945seconds. GPU median35.839ms/p9536.690ms is above
the16.667/20.000/33.333ms timing bands. CPU elapsed medians: skin7.990ms,
pose copy0.637ms, dynamic upload0.405ms, simulation1.548ms, queue submit0.139ms;
fence wait36.193ms and whole frame cycle47.841ms.619 adjacent measured image
presentation intervals have median50.000ms/p9554.167ms. These are not measured
scanout FPS. Parent/child stages overlap and are not additive or CPU utilization.

Current-HAL start AP39.2/BAT29.8/SKIN34.0C, end38.2/30.9/34.7C; framework and
sensor statuses0. This single block does not establish sustained equilibrium
or a gain over a different build/thermal session. In particular, the old12.06ms
or11.73ms skin figures are not matched baselines for7.99ms.

## Retained trace failures

The proposed control/trace/closing-control batch did not complete:

- The traced hold was rejected before sampling because AP34.3C differed from
  the first control by4.9C. The completed hold had returned to the paused menu;
  subsequent staging repeated the original checkpoint warmup. No gate changed.
- The first config-over-ADB-stdin command hung before any Perfetto session
  started. Saved query evidence confirmed no session; only the identified owned
  host client was closed. Its zero-byte output remains local.
- With a config file, Android denied Perfetto's read of
  `/data/local/tmp/horde-scheduler-20261023-r1.pbtxt` (errno13). The299-byte saved
  output is the error message, not trace data. No session started. No alternate
  config location, root, affinity/security change or private shader substitution
  was attempted. Further tracing needs an explicitly permitted supported config
  handoff. This was an Android OS denial, not an automatic approval rejection.

No scheduler events, sampling stacks, per-core utilization, CPU-frequency trace
or detailed GPU-stage split were captured. The existing production-bank stage
probe admission remains a separate source-review prerequisite. Closing control
was not run after the access blocker; there is no tracing-overhead result.
All failed setup/start evidence remains in ignored `reports/phone-trace-20261023-r1/`.

## Custody and remaining acceptance

Released06:20:55UTC. Exact starting Debug fe3a51be and production cc4de768 APKs,
all18 prior save/lock files and preference bytes are verified unchanged. Zero
new save paths. Rotation and font setting1.7 restored unchanged; launcher
foreground, no Debug PID/Horde surface, no active Perfetto session or owned
client/install/transfer. The task's tiny config file was removed. Safe to
disconnect; no further phone operation is planned for this batch.

Exact evidence-head73f02cd9 hosted run39906863727 is terminal cancelled: Android,
Vulkan-host,GCC,Clang and ASan/UBSan pass; MSVC cancelled. This is not an all-green
checkpoint. Existing PR conflicts, Debug30-second route gate, natural earned
journey/recovery and physical/audio/UI acceptance remain open. The phone control
is seeded post-Keeper, muted during timing and not a playthrough.

Audio/haptic manual revalidation required: **NO** for this evidence-only change.
