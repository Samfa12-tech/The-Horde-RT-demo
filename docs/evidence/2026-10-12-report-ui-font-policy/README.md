# Windows report Unicode font-policy candidate

Source parent: `37310bd5b6b423883260c8d9db9c69e988832665`. This is a bounded
Windows report-form repair candidate, independent of the ongoing chapter resource
lifetime work. No game, shader, saved graphics setting, deadline or rendering
budget changes are included.

## Reproduced evidence and attribution limits

[Exact-parent CI](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38168548188)
failed the MSVC report UI test: `WM_SETTEXT` for the approved mixed Unicode note
timed out with error 1460 after 2,002 ms. The other five jobs succeeded. The retained
trace puts the UI thread in RichEdit's GDI text-metrics and character-width path,
with no reported wait-chain cycle. This is a real deadline failure, not evidence
that the unchanged test can be ignored. The earlier EDIT-to-RichEdit replacement
did not eliminate the recurrence.

The same source passed locally before this change (1.85 seconds for the entire
test). A cold-host timeout has not been reproduced locally. The trace supports
investigating Unicode font binding; it does not identify a particular missing
font, prove a deadlock, or prove this candidate fixes the slow path.

## One reversible production variable

The plain-text RichEdit field enables `IMF_UIFONTS`, retaining the other language
options. Windows documents this as use of UI default fonts. Rich formatting,
objects and automatic links remain disabled; script fallback, IME handling,
multicodepage text and exact UTF-16-to-UTF-8 export remain enabled. The surrounding
form's font and layout are unchanged. No warm-up text, sleep, retry, assertion
removal or deadline increase is used.

References: [Microsoft language options](https://learn.microsoft.com/en-us/windows/win32/controls/em-getlangoptions)
and [RichEdit font binding](https://devblogs.microsoft.com/math-in-office/richedit-font-binding/).
The flag is a candidate policy correction; its performance effect must be verified
by current-source CI rather than inferred from those references.

## Focused validation

- New native font-policy assertion failed before implementation on both form
  lifetimes. This red check proves the missing policy, not reproduction of the
  cold-host timeout.
- Debug: report core and native UI tests passed, 2/2; UI 1.67 s, total 4.36 s.
- Release: the same tests passed, 2/2; UI 0.98 s, total 1.02 s.
- Explicit injected 2.2-second UI stall still failed the unchanged two-second
  message deadline at 2,000 ms with error 1460 (exit 1, expected). The failed-run
  evidence is retained.
- Consent defaults, private-input rejection, oversized note rejection, CRLF/CJK/
  supplementary Unicode export, immutable retry and explicit-context assertions
  remain selected. CTest's 30-second ceiling remains unchanged.

Raw local and exact-parent evidence is adjacent under `logs/`. Fresh exact-commit
six-job CI remains the next gate. One passing run is evidence for that candidate,
not a guarantee against every future Windows font-service stall. No phone install,
game launch or owner audio/haptic revalidation is needed solely for this field
policy change.
