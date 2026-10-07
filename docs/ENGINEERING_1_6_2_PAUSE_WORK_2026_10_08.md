# Foreground pause work — 8 October 2026

This affected-device check uses immutable runtime
`ab69537a46248d551420df16f4c3a1f781de6bc5`, Debug APK SHA-256
`99e012534a5b442846d7aeb724dbf17e9ea9860492724c1ee100bdfa20fd3895`.
Actual installed bytes match. Device is SM-S948B / Android 16. Output is
1440×2980, newly traced 720×1490; saved custom 50% / Mobile water and fire /
Glass On / Current shadows / cap30 / Mist On is preserved. This does not change
fresh/reset Glass Off or substitute for the integrated 50/40/33 comparison.

## Corrected observation

Both backends pass ordinary Enter the ruin → Menu → Settings → Back → Resume.
The Debug RT Lab admission mutes semantic audio and publishes the same normal
tuning in every phase. It does not seed a checkpoint, replay or motion scenario.
Autostart is absent. Fresh owned native controls acknowledge each destination;
the ending sample still has the ordinary gameplay Menu control. All preference
entries remain unchanged and both owned applications stop.

Process CPU uses `/proc/<owned PID>/stat` user+system ticks, with the device's
verified `CLK_TCK=100`, stable process start identity and host monotonic elapsed
time. Percentages are relative to **one logical CPU**, not total device CPU.
ADB ownership polling and periodic memory/thermal reads remain observer overhead.

| Backend | Phase | CPU sample seconds | Process CPU seconds | % of one logical CPU |
| --- | --- | ---: | ---: | ---: |
| Pipeline | Stationary play | 4.46 | 0.89 | 19.95 |
| Pipeline | Foreground pause | 180.99 | 5.32 | 2.94 |
| Pipeline | Settings | 60.70 | 1.80 | 2.97 |
| Pipeline | Short resume | 3.41 | 0.93 | 27.31 |
| RayQueryCompute | Stationary play | 4.51 | 1.16 | 25.72 |
| RayQueryCompute | Foreground pause | 180.45 | 5.21 | 2.89 |
| RayQueryCompute | Settings | 60.84 | 1.73 | 2.84 |
| RayQueryCompute | Short resume | 3.37 | 0.94 | 27.92 |

Each native log contains 49 steady approximately-five-second foreground-pause
aggregates with 9–10 render attempts and 248–249 skipped iterations. These are
actual bounded render-work counts. Capability readbacks are periodic and can
retain older completed frames; their serial differences are not used as a
displayed-FPS measurement. The short unpaused samples avoid the guards killing
a stationary player and are not a sustained active-play thermal comparator.

Private runs within the immutable seal are
`pause-work-pipeline-20261007162221-5077bd175411` and
`pause-work-compute-20261007162808-9f729e79831f`. Their CPU/stat, native cadence,
UI acknowledgements, sensor and stop receipts are retained.

## Memory, sensors and limits

Pause-period app PSS samples are 578,035–580,984 KiB on Pipeline and
950,648–1,009,047 KiB on Compute. Initial play PSS is about 599,000 / 1,031,000
KiB respectively; later short-resume endpoints fall to 476,308 / 469,313 KiB.
These transient differences are recorded, not attributed to pause memory savings,
a leak fix, equal backend memory cost or a stabilized integrated-play budget.

Current HAL temperatures at the last samples are AP/BAT/SKIN
31.6/26.6/29.8 °C on Pipeline and 31.8/26.9/30.1 °C on Compute, all status0.
These come from **Current temperatures from HAL**, not older cached callbacks.
The phone is USB-connected. `battery/current_now` reads are permission denied;
the errors are preserved and no permission was changed. No reliable isolated
app/GPU power or controlled sustained thermal reduction is established.

This proves affected foreground work suppression and responsive native
navigation on this exact package. Background suspension, live Graphics preview,
surface/Home recovery, owner feel and sustained performance retain their separate
evidence/gates. It is not a zero-battery or sustained-30-FPS claim.

## Preserved failed setups

The first two longer stationary attempts die in the original opening encounter,
including one ordinary backward gesture that did not escape the arena. Their
Menu guards reject the death screen and no pause pass is claimed.

A third attempt used Debug autostart, which `MainActivity` intentionally uses to
close menus repeatedly for automation. Its Menu tap was followed by gameplay,
then death during the supposed pause interval. Destination acknowledgements were
missing from that helper version. That interval's CPU/cadence is not attributed
to ordinary Pause. All three failed receipts, unchanged preferences and owned
app stops are retained. The corrected helper enters normally and requires the
actual Resume/Settings/Back/Menu destinations before advancing.
