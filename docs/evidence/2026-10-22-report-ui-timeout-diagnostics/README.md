# Repeated Unicode report timeout: diagnostic isolation

Parent: `b610f9980e8aab33116321b13021fa7b41fadb70`. Production remains
restored and unchanged. This checkpoint changes only the native Windows test's
diagnostics; it does **not** establish a fix for the hosted Unicode delay.

## Retained failures and bounded attribution

[Run 39866535522](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39866535522)
failed the MSVC lane on both attempts. The approved original CRLF/CJK/emoji note's
`WM_SETTEXT` timed out with error 1460 after 2,003 ms and 2,000 ms respectively.
Each Windows selection passed 168/169 tests and its separate isolated asset-policy
test; the other five required jobs passed. Exact UI-failure excerpts from both
attempts are retained under `logs/`, with full-source-log hashes in `receipt.json`.
The second attempt was the previously requested unchanged job retry. No further
unchanged retry is part of this investigation.

Both 100 ms samples put the UI in RichEdit/GDI text metrics. The later stack
was already back in `GetMessage`; the wait-chain query reported no cycle. Those
facts neither prove a deadlock nor identify a particular font, cache, service or
missing host resource. The original implementation stays loaded through child
destruction, and creation, fonts, layout and initial focus finish before the test
drives controls. No first-edit lifetime defect or matching local stall was found.
The separate DPI-change font-replacement ordering is not evidence for this
initial-edit failure and is left out of this checkpoint.

The unchanged parent passed locally (native UI 1.13 s; the Unicode message trace
32 ms). The earlier RichEdit, UI-font and fixed-size policy changes did not
eliminate hosted recurrence. Another speculative font-policy change would not
follow from the current evidence.

## Focused diagnostic correction

Previously, the observer suspended/copied the UI stack at 100 ms, resumed it,
then immediately enumerated modules, loaded symbols, unwound frames and printed
them while the same two-second transaction was still being measured.
[Microsoft documents](https://learn.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-syminitialize)
that `SymInitialize(TRUE)` enumerates loaded modules. That diagnostic workload can
compete with the edit. It cannot explain why the edit had already taken 100 ms;
whether it amplified these particular hosted stalls remains unproven.

The observer now copies only into preallocated storage and publishes the immutable
snapshot. DbgHelp and stack output run only after `SendMessageTimeoutW` has
recorded failure. The deadline snapshot and wait chain are collected before
symbol decoding. Successful slow operations retain a small numeric summary after
both action threads join, without a symbol pass inside the timed interaction.

Each snapshot records the suspend/copy/resume API interval and whether resume
succeeded, plus cumulative UI kernel/user CPU time since the matching in-flight
note edit. Unavailable/mismatched CPU baselines are explicit. These are thread
CPU counters, not a sampling profile or proof of a particular wait. The two CPU
samples are cumulative; subtract matched samples to estimate the intervening
CPU interval. Zero reported CPU can reflect timer granularity. Suspension still
perturbs the UI, and unwinding still reads live memory outside the captured stack
range; this is not an observer-free trace or a complete process dump.

The Unicode payload, editing/consent/export assertions, 2,000 ms message deadline
and 30 s CTest ceiling are unchanged. No warm-up text, font installation,
production sleep/retry, input suppression, deadline relaxation or rendering
change is included. The explicit 2.2 s diagnostic sleep remains opt-in and is
never passed by normal CTest.

## Local checks and next gate

The report core and real native report-form tests passed in final Debug and
Release builds (2/2 each; exact timings and test-source hash in `receipt.json`).
The explicit Release 2.2 s stall still failed with error 1460 at 2,002 ms, exit 1.
Its raw copies took 156/201 microseconds; both resumed successfully and showed
the injected sleep stack with zero reported kernel/user CPU. These validate the
diagnostic path and retained rejection, not the hosted font operation's cause.
Earlier local controls, including expected failures, are retained alongside the
final checks. Workspace path spellings in local CTest logs are replaced with
`<WORKTREE>`; test output and failure assertions are retained.

A fresh exact-commit hosted run is required. A green run will show this checkpoint
passed that run; it cannot establish that the recurring native-editor stall is
durably repaired. If it recurs, the deferred early/deadline stacks, suspension
duration and matched UI CPU counters should narrow active work versus waiting
without symbol-loading work inside the gate. Do not keep retrying or invent a
production fix if that evidence still cannot identify a focused defect.

No phone operations, game launch, owner-save/settings edits, merge or release.
Audio/haptic manual revalidation required: **NO** (test diagnostics only).
Existing 1.7 acceptance gaps remain: the Debug route's 30 s gate, natural Keeper
victory through earned journey/recovery, endpoint listening, sustained phone
performance and touch checks. The wall-torch framing candidate remains withdrawn;
the next gameplay investigation must preserve complete held-mesh visibility as
well as physical clearance rather than adopting the cropped-hand candidate.
