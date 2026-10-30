# Windows report Unicode edit correction

Scope: PR27 Windows local report note only. Lower-shadow removal/migration,
shader budgets, Android and the ongoing woodland changes are unchanged.

## Reproduction and attribution

- `d2f3a0419548c8a9435c6f62f9a38115be08a11a`, [CI run 38118661102](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38118661102),
  attempts 1 and 2: MSVC 129/130 passed. The approved Unicode `WM_SETTEXT`
  on note 106 failed with error 1460 after 2004/2000 ms. Both ASCII edits
  before it succeeded; no export/consent assertion was removed.
- The original local fixture passed. Test-only tracing commits did not fix it:
  `50103b119c42bcfca3399e83a86da4a857567242` passed six CI jobs but showed
  516 ms inside the stock EDIT for the first CJK/emoji note, versus 0 ms
  for ASCII edits. A green traced rerun alone was not accepted as a fix.
- `d103a09f792e0eab666649a1ff55e2c4550476a2`, [sampled CI run](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38123277331):
  all six jobs passed, MSVC 130/130. The same edit took 469 ms. Sampling
  after 100 ms found `USER32!EditWndProc` -> `gdi32full!ScriptStringAnalyse`
  -> shaping/layout -> `win32u!NtGdiGetTextExtentExW`, reached from
  `GetMessageW`. This establishes synchronous native text shaping as the
  observed slow path, rather than application validation or unfinished
  dialog initialization. The original timed-out runs had no captured stack;
  their precise OS/font service wait cannot be retrospectively proved.
- No evidence establishes a network download or particular missing font.
  Neither font-cache warming nor sleeps are a correction.

## Correction and preserved contract

Use Windows MSFTEDIT_CLASS for the note, explicitly configured as Unicode
plain text with automatic URL detection disabled. Load only the system32
Msftedit.dll and retain it through child-window destruction. No formatting,
embedded objects, new framework or destination is admitted. Retrieve UTF-16
with explicit CRLF conversion before the unchanged bounded UTF-8 encoder;
this preserves exact JSON/retry bytes despite Rich Edit's CR paragraph storage.

The two-second message deadline and 30-second CTest timeout are unchanged.
Existing consent, private-content rejection, context allowlist, one-shot
export, frozen retry, owner restoration and CJK/supplementary/CRLF assertions
remain. Added checks cover native plain-text/link mode and oversized input.
Test-only tracing buffers successful timings until actions finish; it does not
space out input. A slow-call observer captures a bounded synthetic UI stack;
no note text, object names or private logs are exported.

## Local evidence

- New native plain-text admission assertion failed on the old stock control.
- Corrected Debug and Release native UI fixtures passed, including two
  consecutive form lifetimes (load/destroy/reload of the OS editor).
- Debug Unicode edit: 46 ms; Release: 0 ms at GetTickCount64 granularity.
  These warm local results are not a cold-host performance guarantee.
- Release report core CTest: 1/1 passed, 1.43 seconds.
- Explicit `--exercise-timeout-diagnostic` injected a 2200 ms edit stall:
  failed as expected at 1999 ms, with the sleeping owner stack captured.
  This failing diagnostic is never enabled by the normal CTest invocation.
- The exact pushed correction must pass all six CI jobs; the PR check suite
  and handoff record its result. Diagnostic-only green commits above do not
  establish correction acceptance.

The failing run logs and full stack traces remain in ignored `reports/`.
No game or phone was driven for these UI tests: local forms ran on a separate,
non-visible Windows desktop. Actual Windows file-picker/write, physical phone
CURRENT/HIGHER shadows and saved LOWER-to-CURRENT migration remain pending.
The chapter, visible Kit and resource streaming remain incomplete.
