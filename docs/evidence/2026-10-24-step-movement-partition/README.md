# Bounded step/posture tests, unchanged 30-second limits

The separate Debug movement timeout was reproduced as **accumulated fixture
work**, with no hang observed in the completed diagnostic. One timer-only run
of the original combined sequence completes all 73 checks, zero failures, in
31.610 s. Its measured main body ends at 29.578 s; the remaining 2.032 s outside
that span is unattributed, not assumed to be cleanup. The unchanged registered
30-second timeout is retained as failed historical evidence, not accepted by
giving that combined test a longer deadline.

This is test organization only. Gameplay, ordinary input, route, movement speed,
collision, saves, geometry, pauses and assertion expressions remain unchanged.
No-argument mode still runs the complete original in-process sequence. The
separate terrain-rebind test and previously fixed two-part campaign journey gate
remain distinct.

## Cause and smallest retained organization

The fixture imports actual r23 terrain, runs pure motor/step checks, then four
fresh ordinary-input approaches at its existing 0.8 m/s configuration. No sleeps
are present. The [timed log](profile.stdout.log) records:

| Completed phase | Elapsed main seconds |
| --- | ---: |
| Terrain prepared |1.360 |
| Pure geometry checks |1.371 |
| Live root approach, step/reverse/pause/stale generation |4.696 |
| Full crouch passage/save/load lifecycle |13.695 |
| First rejection fixture; second starts |21.648 |
| Both rejection fixtures |29.578 |

Each rejection repeats a 645-tick approach costing about1.83 s, followed by 382
ticks toward the low-roof/root area costing about5.3-5.4 s. This establishes the
observed workload, not an engine frame-time bottleneck or a general guarantee
that no hang can ever occur. The [timer patch](timer-only.patch.gz) is archived;
timers were removed before the retained implementation.

The two registered selections keep the exact geometry/live-root prefix in both:

- `horde_rt_chapter_step_movement_tests --positive-lifecycle` retains the entire
  crouch toggle/hold, gradual drop, forward/backward/sideways passage, pause,
  same-instance campaign restore and held-input neutral barrier.
- `horde_rt_chapter_step_movement_tests --rejections` retains the too-low passage
  then solid-wall fixtures, with each scoped simulation destroyed before the
  next is constructed. It is registered as `horde_rt_chapter_posture_rejection_tests`.

The original positive simulation and terrain owner remain alive through either
selected child sequence, in the same post-invalidation state. Duplicating the
approximately 4.7-second prefix deliberately avoids substituting a new owner
setup or snapshot handoff. It is not logically needed by the rejection assertions
alone, but preserves their surrounding lifetime. No approach is shortened,
teleported, accelerated or given completion flags. Both cases remain together.

All 64 original `Check` expressions and their order compare exactly. Combined
mode has 73 checks; positive has 67 and rejections 34, including 28 deliberately
duplicated prefix checks. These are source-fixture counts, not natural Keeper
victory or human traversal acceptance. The seed checkpoint is unchanged.

## Actual validation and tradeoff

| Registration | Debug seconds | Release seconds | Deadline |
| --- | ---: | ---: | ---: |
| Positive/lifecycle |15.72 |2.32 |30 s |
| Both rejections |22.30 |1.71 |30 s |
| Paused terrain/aspect rebind |1.53 |0.12 |30 s |

[Debug log](partition-debug-tests.log), [Release log](partition-release-tests.log),
[combined Release 73-check pass](combined-release.log), and [receipt](receipt.json).
Ambiguous multiple selectors fail closed with exit 2 before fixture setup. CTest
JSON verifies every selection/argument and 30-second timeout. Windows selection
retains all three native registrations. The existing Vulkan-host regex and
sanitizer script explicitly include the second registration; the compiled target,
runner, sanitizer flags, permissions and job cap are unchanged.

This gives each meaningful gate headroom but **does not reduce total Debug test
time**: the measured three-test selection takes 39.58 s, with a deliberately
repeated prefix. Release takes 4.18 s. A single local pass does not guarantee cold
runner headroom. The complete original combined Debug sequence still exceeds
30 seconds in this observation; it remains available for diagnosis, not silently
called a passing unchanged gate. No coverage was deleted to conceal the failure.

Local Bash syntax-only checking was **denied at process startup**, before script
parsing: `bash --noprofile --norc -n tools/run-host-sanitizer-validation.sh`
failed MSYS `NtCreateDirectoryObject` with `0xC0000022`. It was not retried and no
permissions/security setting changed. Static selection checks pass; actual
hosted Linux sanitizer execution remains required on the new exact commit.
Do not equate local source inspection with that result.

All 151 parked owner files remain hash-identical. These fixtures use isolated
in-memory simulations, not owner campaign data. No phone, foreground activation,
merge, release or report transmission. Audio/haptic manual revalidation required:
**NO**. Torch/forest source studies remain rejected or diagnostic as recorded.
