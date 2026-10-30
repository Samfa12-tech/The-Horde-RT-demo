# Android Back exit-confirmation acceptance

The finite physical exit-confirmation sequence passed on Samsung **SM-S948B**,
Android **17 / API 37**, user 0, build `CP2A.260605.016.S948BXXS4BZIG`.
This tests the Back change from `ad5857ed` as packaged in source checkpoint
`a447bb731612c45ab494ab63a5ac4a9eac71c810`.

## Exact artifact and custody

The existing active-worktree Debug APK is 183,787,786 bytes, SHA-256
`6d865599c74828bc15165693e7ea23e31454e2909c7ddd0b66123cf0534809d6`.
Local public certificate verification reports the established development V2
SHA-256 `2bc406088b23ac80f2a00a40e2d5d8daa012844dc62f31ad6145d129f5566736`.
Package `com.samfa12.hordelanternrt.debug`, version `1.6.2-debug` and code 10
match the installed package. The on-device pre-install hash is the prior
signer-verified `24e286544a801035a65e1b8f75d7259a27f5792b907d32552bc29882cb5cedfa`
artifact from the [separate checkpoint startup review](../2026-10-18-phone-checkpoint-review/README.md).
`adb install -r` succeeded; on-device hash then matched the current candidate.
The local APK hash was checked again after the test. No extra APK copy/pull was made.

Before installation, no Horde process was running and the launcher was foreground.
Only ADB operated the authorized phone. Desktop input, Reef, the release package,
security settings and owner saves were not targeted. No uninstall, data clearing,
process kill or forced reset occurred.

## Finite sequence and observations

The ordinary entry menu was launched without debug gameplay/capture extras.
PLAY was never pressed. Native Android UI hierarchies supplied the actual
button bounds; app-only screenshot `02-dialog.png` was opened and inspected.

| Step | Observed result |
| --- | --- |
| Entry Back | `Exit The Horde?` / `Close The Horde and return to Android?`; separate `Stay` and `QUIT DEMO`; Stay has focus |
| Tap Stay | Dialog absent, entry PLAY remains; activity does not exit |
| Reopen, then Back | Dialog cancels; entry PLAY remains |
| Reopen, then Android Home and explicit app resume | The existing task returns with entry PLAY and no old confirmation dialog |
| Reopen, explicitly tap QUIT DEMO | Activity/task query contains no Debug Horde activity |
| Relaunch | Clean entry PLAY appears with no stale dialog |
| Terminal cleanup through Back and explicit QUIT DEMO | No Debug Horde activity/task remains; no process kill used |

This validates actual Android Back and ADB-delivered phone button taps, including
default Stay focus, menu retention, background dialog revocation, explicit task
removal and relaunch. A delayed tap on an already detached Java dialog and real
gamepad behavior remain covered by host tests, not simulated as physical-device
events here. Cached app processes may outlive task removal; absence of an
activity is not a claim that Android killed the process.

The app-PID/device-time-scoped log has no fatal exception or signal. The Horde
settings file remains byte-identical at SHA-256
`e3966b09cd9c354cb92833496b7f812139178dd7b2a906a4f809dbf9538c3dee`,
both immediately after installation and after terminal Quit. Before/after
`files/` and `shared_prefs/` path inventories both contain 274 identical paths;
there is no save/campaign/checkpoint-named path. Framework/cache/report contents
were not claimed byte-identical. [Compact receipt](validation.json) records UI
facts and raw ignored artifact hashes. Raw screenshots, XML and logs stay under
`reports/android-back-device-2026-10-18/`.

## Approval review and scope

Automatic approval review **denied before execution** the initial combined
preflight command because it copied/pulled large APKs. Its exact reason was:

> Although the inspection and signer checks are authorized and non-destructive,
> the command unnecessarily copies and pulls roughly 183 MB APK artifacts,
> conflicting with the explicit compact-evidence/no-big-artifacts restriction;
> hash-only verification would suffice.

No part of that rejected command ran. The denied copy/pull was not retried.
The review's suggested hash-only alternative completed successfully using the
existing artifact, prior signer-verified hash and current local public signer
verification. No approval or unsafe fallback remains pending.

Evidence class: exact local Debug installation plus this bounded Android
entry-menu Back/Stay/cancel/Home/resume/Quit/relaunch behavior. This is **not**
root stepping, crouch/headroom, gameplay Save/Load, process-death save recovery,
POSIX/physical power-loss durability, Query, sustained performance or owner
visual/audio/haptic acceptance. The earlier checkpoint207 evidence remains tied
to its older APK. No additional startup or performance loop was run.

Audio/haptic manual revalidation required: **NO additional requirement from this
menu acceptance pass**; no new playback or haptic implementation was made here.
This pass changed no application source or scripts; the checkpoint records
compact evidence only.
