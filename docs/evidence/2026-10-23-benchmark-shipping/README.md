# Windows Shipping benchmark course and rollback - 23 October 2026

Base source: `53cbb8607ef9de769035223435582b63a4bd9e9c`.
The provisional Windows CLI now has an actual optimized **Shipping/High full
course, local report and exact restoration pass**. This is one platform's
acceptance slice, not official profile adoption, ranked fairness, sustainable
performance, Android acceptance or the natural 1.7 campaign journey.

## Actual run and report

The existing `windows-x64-release` preset builds the native app with its normal
Shipping/High shader policy, no overrides and actor worker OFF. No shader,
renderer, quality default, asset, save schema, gameplay or playback code changed.
The staged EXE uses a new muted isolated INI and the existing assets junction.
The existing unattended benchmark opens no campaign session. Protected source
EXE/settings, production player/arms/Kit assets, isolated INI and the normal owner
campaign location are compared before/after. All comparisons pass; the checked
owner campaign location contains zero files. Owner files are not copied into Git.

The invocation uses the existing authored workload, with no lifecycle-only,
checkpoint, capture, workload-mutation or hidden-campaign flags:

```text
<staged-Release-exe> --benchmark-showcase <new-local-output-directory> --benchmark-profile windows-rt-provisional-v1
```

Both full processes finish normally with exit 0. Each completes the default two
laps and all 26 waypoint visits. The first lap warms up; all **1,838 second-lap
rows** are measured and completed, CPU accepted and GPU valid, with zero rejected,
cancelled, missing or outstanding rows. Every row has:

- Actual internal **1920x1080**, output **1920x1080**, aspect **1.77777779**,
  transform 0, DRS off, no blit and preserved aspect.
- Matching submitted/completed identity, strictly increasing completion serials,
  the same measured epoch/generation, successful RT presentation and valid CPU
  stage evidence. Shipping Diagnostic counters remain compiled out/null.
- Current shadows with actual 1/1 primary local/sky samples, High fire with
  10 volume steps and two reflection samples, mist on and Low dust. The selected
  immutable Shipping/High pair remains unchanged across the report correction.

The final local JSON parses successfully and its embedded profile-attempt object
equals the separately saved lifecycle receipt. It remains **custom-unranked,
ranked=false**. No report, hardware payload or email is automatically sent.
Raw reports, per-frame rows, custody details and binaries remain ignored under
`reports/benchmark-shipping-20261023/`. Only sanitized aggregates and hashes are
backed up here.

## Demonstrated report defect and focused correction

The first full course on exact `53cbb860` passes traversal, dimensions and RT
restoration, but its Windows tuning attachment incorrectly says
`policyStable:false` and `shadowPolicySource:unavailable`. Its raw report is
retained, not edited or substituted with the corrected result.

`BuildWindowsBenchmarkTuningJson` still validates the fourth quality word as a
mist-only 0/1 flag. That word already packs mist in bit0 and dust in bits1-2.
The provisional Low-dust/mist-on value 2 is legal. The report validator rejected
it even though completed-row evidence correctly records the uploaded policy.
This is a reporting defect, not evidence of a renderer policy change.

The correction reuses `ResolveUploadedMistEnabled` and
`ResolveUploadedDustQuality`, then validates the complete reconstructed word.
Mist disclosure uses the decoded bit rather than comparing the entire word to
zero. Unknown encodings and inconsistent shadow budgets remain rejected;
different start/end words still invalidate stability. No assertion is removed.

The focused native fixture reproduces **four failed checks** before correction.
It now covers all six valid mist/dust encodings, including both mist states with
Low/Standard dust, and rejects a dust change at the endpoints. Its existing
unknown-flag witness changes from 2 (now an admitted dust encoding) to 6 (the
first unknown encoding). After correction the same selected Release CTest
passes **1/1 in 0.99 s**. The app is rebuilt and only the affected full course is
repeated; unrelated suites and the previously passing cancellation/error cases
are not rerun.

The corrected course's tuning attachment now truthfully reports uploaded
Current shadows, 1/1 samples, mist on and stable authored/High endpoints.
The independent local validator checks every row, not just those endpoints,
and recomputes the GPU median from raw row durations.

## Exact restoration and timing scope

Actual application and restoration are acknowledged by their owning completed
RT frames. The original 75% tuple is deliberately distinct: **924x602 internal
to 1232x803 output**, aspect **1.53488374**, linear blit and
`aspectPreserved:false` from ordinary integer rounding. It returns exactly,
along with Mobile water, Low fire, Higher shadows, mist off and Standard dust.
Original live intent/confirmed settings, HWND style/exstyle, menu, placement,
outer/client rectangles and fullscreen bookkeeping all pass exact restoration.
The scope protocol also checks the live glass allocation under the same epoch.
No settings file write is permitted during either profile run.

The corrected measured lap observes a GPU whole-command-buffer median/p95 of
**4.5071 / 6.6267 ms**, and CPU `WholeFrameCycle` median/p95 of
**7.9372 / 10.9755 ms** on the RTX 5050 Laptop GPU. These describe this authored
tomb course only. GPU duration includes acceleration-structure/trace/copy work;
it is not isolated ray dispatch. CPU frame-cycle duration includes fence/driver
waits; it is not CPU busy utilization or measured scanout FPS. MAILBOX does not
itself establish display cadence.

The two process wall durations (64.781 and 36.890 seconds) include startup,
loading, warmup and restoration; they are **not a matched optimization result**.
No thermal/power/clock or scanout trace was collected, and no outdoor, phone,
sustained warm-play or image-quality gain is claimed. The legacy product build
string remains `1.6.2`; development source is identified by the base commit,
changed source blobs and exact executable/pipeline hashes in `receipt.json`.

The residual MSBuild node from completed builds was observed idle with unchanged
CPU before the corrected native run; no compiler/link or other Horde renderer
was active. It was left untouched. Both owned game processes have exited.

Audio/haptic manual revalidation required: **NO**. Only report decoding changes;
semantic events, event-time listener/source data, playback and haptics do not.
The native runs use muted isolated test settings and establish no listening pass.

## Remaining acceptance

The preceding exact `53cbb860` [CI39966365867](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39966365867)
passes all six jobs. MSVC passes **150/150** in **148.62 s** of CTest;
configure/build/selection take 34/622/150 s and the job totals **14m16s**, only
44 s below the existing cap. APK resolver passes 10/10; annotations are empty.
This proves that source's hosted completion, not robust future headroom or the
corrected report's acceptance. Earlier cancellations remain retained.
`53c-terminal-receipt.json` records the exact terminal source and log hashes.
The two-file report correction needs its own hosted readback after backup;
local results are not labelled hosted passes. No CI cap or coverage changes.

Android fixed-output launch, rotation/background restoration, practical profile
resolution/image quality and sustained performance remain open while the phone
is disconnected. No phone access or reconnect attempt occurred. Profile/version/
backend cohorts, user consent, transparent sample populations and outliers,
official ranked profile admission and any leaderboard publication remain
separate work. Existing PR conflicts, Debug route deadline, natural earned
journey and deferred forest/smoke/flask decisions remain unchanged.

Stop at this verified Windows checkpoint. The next benchmark milestone is the
Android fixed-output/profile lifecycle when the owner makes hardware available;
independent previously approved 1.7 feature work can continue under the parent.
No merge, release, deployment, email, report upload or public leaderboard change.
