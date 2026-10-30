# Earned Android checkpoint: first record creation failed

**FAILED / recovery acceptance blocked.** On SM-S948B, normal touch play reached
the lower lantern claim, but the first Slot 1 autosave and a subsequent Slot 2
manual save both failed with native phase 8, public error 6 and durability 0.
No `.hrd` record was created in the new isolated namespace. This receipt preserves
that failure; it does not certify persistence, ascent, reload or death recovery.

## Exact candidate and scope

Installed Debug APK SHA-256:
`e9ce289a8a527083aa1d6d2ca2976da5ba439768f8cc933b3a34e20f153701c8`
(183,958,042 bytes), package `com.samfa12.hordelanternrt.debug`, code 10.
The exact selected source hashes, build attempts and four-ABI artifact are in
[the build receipt](../2026-10-18-android-death-load/android-build.json), based on
parent `0a9d75882f6e64fc2f2b57465733bb63d6feb825` plus its recorded source changes.
That parent alone is not an immutable source identity for this APK. Installation
and signer evidence is hashed in [validation.json](validation.json); later output
at the reused Gradle APK path does not identify this run.

Device: SM-S948B, Android 17 / API 37, build
`CP2A.260605.016.S948BXXS4BZIG`, user 0. Test PID 31216 used only
`files/campaign-tests/earned-20261018-r1`. The ordinary opening, crouch passage,
guards, Keeper, chest and lantern were reached through touch play, with damage
enabled. No checkpoint was imported or seeded into this earned journey. The
existing read-only GPS/input inspector was enabled in the same activity; it did
not author progress or turn this into a seeded route.

## Observations retained

All six selected screenshots were opened during receipt review. They remain
ignored raw files under `reports/android-earned-recovery-2026-10-18/`; exact
paths, sizes and hashes are recorded in the JSON, without adding large images
to Git.

| Screenshot | Observed fact |
| --- | --- |
| `57-keeper-final.png` | Keeper/lich dead, HP 0; player alive, vitality 3/3, damage ON |
| `71-chest-close.png` | Actual OPEN CHEST interaction prompt |
| `73-lantern-available.png` | Open chest and TAKE LANTERN prompt |
| `75-lantern-claimed.png` | Held illuminated lantern and LOWER action after claim |
| `77-save-menu.png` | Correct TEST SAVES ONLY namespace; Slot 1 and Slot 2 manual/latest-auto summaries empty |
| `78-manual-result.png` | Storage failure message; summaries remain empty and manual controls disabled |

The gameplay captures show `RT RayTracingPipeline | presented YES`, 3/3 hearts
and the saved 40% scale (576x1188 into 1440x2971). This is current RT presentation
evidence at those captured moments, not a sustained performance or quality
acceptance result. No render setting was changed for this run.

`pid-31216-terminal.log` records the first autosave failure at 15:01:43.119:
request 2, generation 1, phase 8, error 6, isolated 1, durability 0. The manual
attempt fails at 15:05:23.020: request 4, generation 1, phase 8, error 6,
action 2, isolated 1, durability 0. The public error does not identify the failed
filesystem operation or OS error. The scoped PID logs contain no `avc:` denial;
that does not prove SELinux or any other cause absent. The creation failure's
cause remains unproven in this candidate.

## Terminal custody

The app was explicitly Quit; root reported zero remaining activities, and the
saved log records surface destruction. A dedicated activity-count dump was not
saved in this artifact set, so that count is a root observation rather than an
independently replayable file assertion.

Before and after install inventories contain the same 281 paths. Terminal
inventory retains all 281 and adds only two zero-byte locks in the earned test
namespace: `.slot-1-auto.hrd.lock` and `.slot-2-manual.hrd.lock` (283 paths total).
The terminal directory listing contains no `.hrd` records. The six prior isolated
test records match their previous recorded hashes. Settings remain exactly
`e3966b09cd9c354cb92833496b7f812139178dd7b2a906a4f809dbf9538c3dee`
before install, after install and at terminal capture. Path retention is not a
bulk content hash of every app file. No owner campaign record was created.

## Separate diagnostic and remaining gate

Root's subsequent candidate
`00f196f24eb07cc4eacf4e096f05496912b069f79505f98350812ada657a42b1`
uses the separate `create-diagnostic-20261018-r1` namespace and a copied prior
test record to diagnose fresh file creation. Its result and subsequent repair are
[recorded separately](../2026-10-18-android-create-publication/README.md).
Its seeded state cannot replace this earned failure
or certify an earned journey; its minimal storage-failure logging does not
change the storage implementation.

The next gate is to explain and fix fresh record creation, then validate that
fix on an exact artifact before claiming earned autosave/manual durability,
fresh-process reload, death Load, ascent or recovery. Those cases were not run
after this material storage failure.
