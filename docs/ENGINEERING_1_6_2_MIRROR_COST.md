# Keeper mirror cost comparison - 5 October 2026

Defer the stone fallback, mirror removal and a mobile mirror option. The bounded
comparison did not demonstrate a useful repeatable saving. Retain the ordinary
mobile mirror; the owner prefers it, and desktop/high graphics must retain it
regardless of phone cost. This closes this experiment without changing runtime,
graphics defaults or release authority.

## Exact scope and admission

The isolated source uses runtime `6468c3e47908901e8a904cf4a0eb1c7029c9767b`.
Only the Keeper-room quad's authored material changes from `SurfaceMirror` to
the existing `SurfaceDryStone`. Its vertices, triangles, normal, frame and wall
remain identical, as do the other two mirrors, torches, assets, shaders and
Current secondary transport. Ordinary stone still uses a genuine RT diffuse
bounce; this is not a skip-all-bounces experiment.

The installed Shipping Benchmark control APK is
`c17e219dadd44abd047d4ff5fb3142cc577209a137088a03aaed58a492541e18`;
the stone APK is
`982619a7892f5001cfb9ed78e17ea2a2547e0fb42d962c44f321a81737221ce6`.
Actual package/module admission joins four ABI ELFs per APK, 16 KiB load
alignment, full notices and embedded SDK-validated modules. Both use O2,
Shipping/Mobile, minimum scale 50% and no override below 50% or checkpoints.
Compiler inventory was collected after both builds, not before every command.

The phone courses use Pipeline at 75%, 1080x2235 internal/1440x2980 output,
Mobile water/fire/dielectric, Glass On, Current shadows and cap 30 Hz.
Each completes one warmup and one measured lap with 1,838 admitted owning rows;
the four-course total is 7,352. Native reports declare completed route/workload
and presentation of every frame. Per-frame settings, quality strategy, light
records, fixed 60 Hz ticks and population join exactly across the four courses.
The recorded maximum is two active fire emitters.

## Observed durations

These are arithmetic means of the admitted raw RT GPU durations, not frame
medians, scanout FPS or sustained gameplay rates. Pooled populations contain
3,676 rows per material and 202 finale rows per material.

| Ordered course | RT GPU mean ms | Finale RT GPU mean ms | Thermal status start/end | Battery header C start/end |
| --- | ---: | ---: | --- | --- |
| Mirror A1 | 76.727918 | 145.536208 | 0 / 0 | 32.2 / 36.6 |
| Stone B1 | 75.102354 | 137.384000 | 0 / 1 | 34.2 / 38.7 |
| Stone B2 | 77.226416 | 138.683936 | 0 / 1 | 36.2 / 38.9 |
| Mirror A2 | 63.229957 | 116.662857 | 0 / 2 | 36.1 / 42.9 |
| Pooled mirror | 69.978937 | 131.099532 | Unequal context | Unequal context |
| Pooled stone | 76.164385 | 138.033968 | Unequal context | Unequal context |

Stone minus mirror is descriptively +6.185448 ms (+8.839%) across the course and
+6.934436 ms (+5.289%) in the finale. The final mirror control's 63.229957 ms is
below both stone means 75.102354/77.226416 ms. This ordered result supports
deferring the proposed fallback; it does not establish that stone causes a
slowdown or that mirrors are free.

## Limits and retained decision

Battery and thermal headers are external observations, not ASIC temperatures
or evidence of equal clocks, throttling, cooling or power. These contexts differ
despite identical recorded workloads. There is no dynamic material-ID readback,
ray/query counter attribution, 50%/33% end-to-end comparison, separate Compute
cost result, moving-image adoption or sustained combat/FPS acceptance.

The existing stone ORM census, including decoded ASTC, lies below the existing
0.38 reflection threshold. It supports the intended material path distinction;
it does not count skipped queries on the device. All 13 diagnostic control
captures match the recorded current-main RGBA images; geometry/light records
join the stone captures. Static image evidence is separate from phone cost.

The sealed private comparison receipt has SHA256
`cb5190f66ef655d4069262398e270724bcce40372ada14e8cd5c33f03f786248`.
Exact source, artifact, material and admission digests remain in the
[inactive experiment record](experiments/1.6.2-mirror/2026-10-05/source-record.json).
Raw reports, screenshots, launch records and preferences remain private.
Reopen only for a materially different justified experiment and matched
evidence; do not repeat this comparison merely to seek a favorable number.
The separate [Keeper torch cost](ENGINEERING_1_6_2_KEEPER_TORCH_COST.md)
comparison measures another change and is not offset by this result.
