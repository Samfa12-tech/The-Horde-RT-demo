# Detached benchmark cancellation: harness coverage correction

**The opening-menu feature remains WIP/unadmitted.** This bounded follow-up to
`67c31888fd5d9b467806181bbd148b385ecc9ba8` resolves the missed *cancellation
witness*. It does not admit visible layout, physical/controller interaction,
listening, final Release interaction or every close/error boundary.

The later [WM_CLOSE follow-up](../2026-10-23-benchmark-close-boundaries/README.md)
records a separate missing successor close-intent receipt, its focused fix and
two bounded close cases. It preserves this cancellation receipt and does not
admit the full feature or the remaining physical/Release gates.

## Demonstrated cause

The original [failed gate and immutable receipt](../2026-10-23-benchmark-menu-wip/README.md)
polled `GWLP_USERDATA == 0` from another process and then used synchronous
`SendMessageTimeout` for Esc. Observing null did not reserve that interval or
establish when the owner thread would service the message. The native teardown
contains no ordinary message pump between detachment and successor construction.

One instrumented observation retained the same required `detached-cancel`
assertion. It again failed that assertion, **and recorded Esc delivered with
`context-attached=1 phase=2`** (Benchmark), despite the harness having observed
null. Cancellation was safely handled by the attached benchmark guard; the app
returned to Entry and exited normally with custody unchanged. This demonstrates
a time-of-observation versus time-of-delivery coverage race, rather than evidence
that the detached bridge lost the input or referenced a retired renderer.

## Minimal correction and exact evidence

Debug's explicitly isolated `--benchmark-menu-validate` fixture now accepts
`--benchmark-menu-detached-cancel`. A one-shot hook runs immediately after
`GWLP_USERDATA` is cleared, only for Queued Entry retirement and the matching
stable HWND transfer property. It sends ordinary `WM_KEYDOWN/VK_ESCAPE` through
the **real native window procedure**. It does not write cancellation state,
grant a retirement proof, bypass profile logic or synthesize a completed frame.
The normal bridge, owner teardown, next-owner cancellation and fresh Entry RT
presentation still have to work.

The final corrected targeted check passed in **10.343 s**, with this actual ordering:

```text
detached-inject context-null=1 phase=queued pending-before=0
escape-delivery context-attached=0 phase=1
detached-cancel
detached-inject-result pending-after=1
entry-retired=1 successorAdmitted=1
entry-presented campaignAttached=1
```

The final `campaignAttached=1` is the returned Entry's isolated menu facade, not
a campaign attached to Showcase. The native profile receipt reports
`campaignAttached=false`, `cancelRequested=true`, no apply acknowledgment, no
measurement success, no ranked result and no settings writes. Since cancellation
preceded apply, there is no profile-restore acknowledgment to claim. Fresh Entry
still requires actual completed RT presentation and equality with the original
Entry configuration. The native outer window/style also matched exactly, and
staged/source settings plus owner campaign hashes were unchanged.

Final Debug and Release app builds pass. Two early startup guard checks exit 2
as required: the flag alone requires isolated mode; Release rejects validation
as Debug-only. Settings remain unchanged/absent as before. No production menu,
retirement, input routing or rendering behavior was changed by this correction.

The base WIP's exact [CI40022045914](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40022045914)
is terminal success on attempt 1 at `67c31888`: all six jobs pass (MSVC 787 s).
That validates the base checkpoint, not this newer local native witness; the
correction's hosted checks must be read against its own later commit.

Read-only review identified two additional validator defects, both corrected:
the runner now requires native exit code zero after writing its receipt, and
isolated startup uses `SW_SHOWNOACTIVATE`, skips native focus/foreground calls,
suppresses automatic update checks and logs rather than opening error dialogs.
The first 8.250 s witness and two intermediate successful checks are retained,
but only the final run includes all these safeguards.

Reproduce the **background, muted, isolated native check** from the worktree
after building the Debug app, with a new label each time:

```powershell
python tools/validate-windows-benchmark-detached-cancel.py unique-detach-label
```

The optional `--observe-original-poll` mode preserves the old polling gate and
delivery trace for diagnosis; it is intentionally not a reliable passing test.
The tool has no foreground-activation, screenshot or phone path. Final isolated
native startup also suppresses those activation/prompt routes. It requires
the real detached-handler witness, rejects profile application or a success
report, verifies fresh Entry/custody and closes only its owned application. It
does not automatically kill an unresponsive process or retry a failed run.

## Limits and next blocker

Both original red results remain retained, with their identities in the
[compact receipt](receipt.json). The first receipt is unchanged. This is a
synchronous, deliberately timed native handler/lifetime check, not proof of
physical input timing or all asynchronous delivery schedules. Close/Alt-F4 and
other ownership phases remain separate acceptance cases.

The earlier Python foreground request returned FALSE and was not explicitly
retried. Review found that earlier native startup still requested activation;
its return value was not recorded. Historical `foregroundRequestAttempted:false`
therefore describes Python calls only, not native startup. These earlier records
are preserved with that limitation; the final isolated path suppresses native
activation as described above. No alternate foreground route was used.
Visible Windows layout/DPI, physical/controller interaction and transition
listening remain pending. The benchmark remains WIP until these remaining gates
are addressed. No phone, torch edits, report transmission, merge or release.
All 151 parked owner files are preserved. Local test/render/build commands end
at this checkpoint. Audio/haptic manual revalidation remains **YES** for the
underlying new menu transition; muted checks cannot establish it.

Detailed receipts/logs remain in ignored
`reports/benchmark-menu-entry-20261023/`. The previously recorded bounded torch
orientation/hand-target proposal remains the next separate runtime proposal;
this cancellation investigation makes no torch change.
