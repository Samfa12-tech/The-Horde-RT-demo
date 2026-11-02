# Windows repeat-death menu repair - 2 November 2026

Owner observed healing work, death to the lich, Retry, then a second death with
no Retry overlay and an apparently frozen game. The running private a004 flask
UX executable was SHA256 `0e667e14a22e802bb704d4b9da8b7f0f4a1e5edce8a801b86152f12b0d023e03`.
No input, close, kill or relaunch was sent during preservation. The owner later
closed it explicitly; its original package/state remains untouched.

The Windows process was responding, with38 threads and increasing CPU samples.
A private1,387,276-byte Windows DbgHelp thread/context snapshot was retained under
ignored `reports/windows-second-death-20261102-47972/`, with SHA256
`72200ba43e2131db564c46518ad52d6fc3d9a263734ea805ee92c49a8dcee607`.
It is not a full-memory dump and is not published. Accessibility evidence showed
zero vitality without a Retry card. An unrelated-desktop screenshot did not
establish game pixels and is not used as visual proof. No resolved thread-stack
analysis proves or excludes an independent render/driver wait.

## Cause and fix

The immediate pre-claim Windows Retry path hid `pauseMenuVisible` but retained
`deathOverlayVisible`. The next death returned early from ShowDeathMenu because
that stale latch was already true. This directly reproduces the missing second
menu in the production transition fixture and explains an apparent input/UI
freeze. It is not a claim to have diagnosed every possible deadlock.

The exact production ShowDeathMenu and ApplyPlayerRetryCheckpoint bodies now
live in `windows_player_retry.inl`, included by the application and focused host
fixture. Retry retires the death latch only after the shared owner publishes a
changed retry generation, Alive phase and positive vitality. Invalid, failed,
uncommitted or zero-health retries retain the existing death UI. Pending
confirmed-record retries remain paused behind their existing separate barrier.
Held desktop/controller input is cleared before retry; successful admission
returns gameplay focus. Death presentation can repair a stale visible latch
whose pause menu is hidden, while an already presented card remains idempotent.
Existing successful campaign-load commit/barrier behavior is unchanged.

## Checks and exact candidate

The direct production-function fixture passed23 checks: three death/retry
cycles, missing/invalid destination, failed/uncommitted/zero-health retry,
held-input/focus/reset callbacks, stale-menu repair and pending confirmed-record
ownership. Its injected OS/simulation outcomes are fault-injection host evidence,
not real RT gameplay or physical focus acceptance. The registered Release CTest
passed1/1 and the private Windows Release application build passed.

Separate unlaunched candidate:
`reports/horde-windows-acceptance-20261102-retry-fix/HordeLanternRT.exe`,5054976 bytes,
SHA256 `8dea404dc4f055ce8fb85757ad808900be149627144a4371dce7282ae3f7368c`.
Private receipt `reports/windows-second-death-20261102-47972/retry-fix-receipt.json`,
SHA256 `90996fe1c0946b89eddc52bf22220846b4cda1c2d5ab912975ed5c55dd51ed1a`.
Built from942644cc plus this fix; actual embedded build metadata is not relabelled
to a later commit. All151 parked owner hashes are unchanged. No Android/native
asset, gameplay balance, save format, public admission or transport change.

Physical Release death→Retry→death→Retry, menu focus and input resumption remain
an owner validation step. The compact dynamic flask HUD and dodge-forgiveness
evaluation are separate follow-ups, not part of this repair.
