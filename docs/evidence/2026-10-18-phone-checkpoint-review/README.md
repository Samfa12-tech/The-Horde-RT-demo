# Bounded physical checkpoint startup review

On 18 October 2026 the owner made his phone available for automated testing.
This closes one bounded reproduction attempt against the historical checkpoint
207 tick-zero staging failure. **That failure did not recur; its cause remains
unattributed.** A passing later reproduction does not establish that it was fixed.

## Exact candidate and device

- Source: `0d6c22d698604d527b85fddb33f6db1fadedf2ed`, existing
  `codex/horde-1.7-wp2-vertical-proof` worktree. The APK was sealed by the
  [combined review](../2026-10-18-campaign-combined-review/validation.json),
  copied to ignored reports before concurrent host bridge development, and not
  rebuilt during this test.
- Separate Debug package `com.samfa12.hordelanternrt.debug`, version
  `1.6.2-debug`, code 10, 183,665,114 bytes, SHA-256
  `24e286544a801035a65e1b8f75d7259a27f5792b907d32552bc29882cb5cedfa`.
- Samsung **SM-S948B**, Android **17 / API 37**, build
  `CP2A.260605.016.S948BXXS4BZIG`, security patch `2026-10-05`, user 0.
  This fresh OS identity supersedes older Android 16 descriptions for this run.
- Adreno (TM) 840, Vulkan 1.4.295, raw driver 2150932499, Qualcomm driver build
  `089d643e2a`, conformance 1.4.1.0.
- Native RayTracingPipeline with production-bank Diagnostic/Mobile modules.
  Saved 40% scale retained: 576x1188 RT dispatch, 1440x2971 swapchain. CURRENT
  shadows, Mobile water/fire, glass off, mist on, dust 1. No quality override.

The installed Debug APK before update was SHA-256
`d0363ad6649aecdf3482fa2de7e6d635a8b4a23c2b101fb04929372723817dd0`.
Both APKs verified against public V2 certificate SHA-256
`2bc406088b23ac80f2a00a40e2d5d8daa012844dc62f31ad6145d129f5566736`.
Package/version compatibility was inspected before `adb install -r`; installation
succeeded and the installed APK hash matched the sealed candidate. No signing
material was accessed. The release package was not targeted.

## Two finite cases

Horde was not running before the test and the launcher was foreground. Each
case starts a fresh Debug process with `--user 0`, transient `horde.debug.checkpoint`,
`horde.debug.capture=true`, `horde.debug.autostart=true`, overlay off and GPU
timing on. There is no scale, RT Lab, save or load extra. Each wait retains its
original 110-second bound and app-PID/device-time-scoped log collection. Neither
case needed a retry or a longer deadline.

| Cold case | Result | Current completed RT frame |
| --- | --- | --- |
| `chapter-kit-reunion` / 207 | `capture-ready`, 12 stable presented frames; outside, X -28.7023 / Z -5.8033, yaw 1.5708, pitch -0.10; alive 3/3 | scene epoch 3, measurement generation 3, record/submission/completion/present 12, simulation tick 1436 |
| ordinary `opening` | `capture-ready`, 12 stable presented frames; opening, X 0 / Z 1.85, yaw 0, pitch -0.05; alive 3/3 | scene epoch 2, measurement generation 3, record/submission/completion/present 12, simulation tick 0 |

Both owning frame receipts report scene ready, RT dispatch recorded, swapchain
copy recorded, valid fence-owned diagnostics/GPU timestamps and successful
presentation of that submission. Neither app-scoped log contains a checkpoint
stage failure or fatal crash. The new failure fields therefore had no failed
admission to diagnose. Do not infer a successful terrain-generation comparison
from their absence. Native initial capability snapshots retain their own pending
completed-frame observer; current completed-frame proof comes from each later
`showcase_debug_state.json`, not from that early snapshot.

The two actual foreground screenshots were opened: checkpoint 207 shows the
close Kit/lantern view against the night forest, and opening shows the held
torch, stone route and encounter placeholders. This is image inspection of
these frozen views, not owner visual acceptance or a full chapter playthrough.
Raw screenshots/logs remain ignored; hashes and compact owning-frame extracts
are in [validation.json](validation.json).

Latest owning GPU durations were 187.054318 ms at reunion and 19.715416 ms at
opening. These are individual Diagnostic frozen-view samples, **not** a median,
sustained FPS, or matched performance comparison with earlier Shipping runs.
No performance optimisation loop was run. This evidence does not close the
known live-light/exterior performance gate.

## Data custody and limits

The test used only existing transient capture state and report outputs. No save
operation was invoked; the tested source predates the real worker/storage
bridge. Before and after, the sorted inventory of 274 `files/` and
`shared_prefs/` paths is identical, with no path named save/campaign/checkpoint.
The Horde settings file remains byte-identical at SHA-256
`e3966b09cd9c354cb92833496b7f812139178dd7b2a906a4f809dbf9538c3dee`.
The other two WebView preference hashes are unchanged. Android's framework-owned
`android.app.ActivityThread.IDS.xml` changed during launch; it was not restored
or described as owner settings. Existing diagnostic report contents may update
normally. No claim of whole-app-data byte identity is made.

Only these isolated Debug processes were force-stopped: between the cases and
after the final capture. This terminated application processes; it did not clear
data. Debug is stopped at completion. There was no uninstall, app-data clearing,
save deletion, device reset, security-setting change or interaction with another
app. No owner-running process was displaced. One initial unqualified `pm list packages`
inspection hit a user-150 permission error; explicit user-0 activity commands and
`ps -A` inspection resolved scope without changing user/security settings.

Evidence class: exact local Debug in-place installation and two bounded physical
Pipeline startup/checkpoint presentations with inspected screenshots. Query,
touch/menus, process-death save recovery, Home/resume, integrated Save/Load,
POSIX directory-flush or physical power-loss durability, sustained warm pacing
and owner audio/haptic/visual acceptance remain **NOT RUN**. Audio/haptic manual
revalidation required: **NO additional request from this frozen capture review**;
the implementation's existing integrated load/feedback acceptance gate remains.

Next device work should use a separately sealed candidate after the host-tested
worker/load path is integrated, with an explicitly isolated test save namespace
and before/after custody checks. Do not treat this checkpoint capture as a save
round trip or launch another unbounded performance loop.
