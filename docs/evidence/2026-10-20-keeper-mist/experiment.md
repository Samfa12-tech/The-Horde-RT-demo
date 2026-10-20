# Keeper mist finite comparison, declared before phone runs

Existing WP2 worktree, starting commit eaffada89b02cd0ce542a622ee656f67ae989952.
Only this isolated noncombat Debug request is used. The shared simulation runs
430 ordinary local setup ticks (torch failure, then live reveal), with damage
disabled, then freezes its value snapshot. Camera (-31.2, support -0.95, -15.2),
yaw -pi/2, Keeper health 3, full room mist density 1, staff discharge 0. Production
world torch lights 3 and 4 remain active through the normal renderer adapter.
This differs from moving combat: no player combat edges, enemy movement, changing
pose, live input, companion traversal, extra staff light or campaign IO. It does
not certify the physical fight, dodge/parry, sustained FPS or natural traversal.

SM-S948B R5GL219SZGK, accepted saved scale 40%. Shipping RT instrumentation,
Mobile optical profile, RayTracingPipeline, route timing enabled; no stage/clock
probes. Saved water Mobile, fire Mobile, cap 30, glass Off, shadow Current,
dust Low stay fixed. Only the transient Mist bit changes. Owner settings and all
12 existing campaign records/locks must remain byte exact; install uses matching
Debug signer and replace only, with no data clearing. No Release app operations.

Order: A1 Mist On, B1 Off, B2 Off, A2 On. Four timed runs maximum; two preparation
rejections maximum, no blind repeats or source changes within the comparison.
Each request has a unique output identity, 12 preparation frames and a 30-second
arming window, followed by 60 warmup plus 120 retained sample frames. Warmup and
samples must have owning completed real RT/present evidence, two uploaded lights,
the exact frozen tick/settings and one matching actual presentation row each.
The host rejects missing, duplicate, out-of-scope or collector error evidence.

Native rendering serializes each frame with device idle. Thus whole RT GPU
duration can compare this fixed workload; display intervals describe this
serialized diagnostic and are not gameplay FPS. CPU stages are separate and are
not added to overlapping GPU/fence times. A single native image is captured only
after all timed frames and the presentation drain. No images during measurement.

Before each run the app must be idle and the current HAL framework status 0.
A1 defines the thermal reference after preparation. Later prepared starts require
AP, BAT and SKIN within 1.5 C of A1, identical per-sensor statuses and framework 0.
Passive cooldown is bounded to 180 seconds; before relaunch, BAT/SKIN must be no
more than 0.5 C above A1 prepared values and AP no higher than its prepared value.
The subsequent native 30-second arming bound is not extended. End temperatures
also need all three within 1.5 C and status 0 for paired admission. Capture current
HAL before/after and every 10 seconds; cached temperatures are not substituted.
USB charging state is recorded, GPU clocks/power are not measured or fixed.
If a closing control cannot be admitted, preserve that limit and narrow claims.

Accept a descriptive Mist saving only with unchanged scene/settings/lights and
valid owning GPU/display joins plus thermal admission; target >=3% median GPU
reduction in both Off observations against admitted controls. Otherwise record
no useful measured gain. Mobile default Off is the owner's selected product
choice regardless; explicit historical saved values remain preserved.
No render scale reduction, Fire/Dust default change, shader rewrite, public push,
release, outside-content expansion or CPU scheduling change is part of this slice.
