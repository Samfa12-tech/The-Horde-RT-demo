# Unicode report-field size policy

Parent `f005a4ecd4f3c45ed91d2ab908372b9c48e191cd` failed the MSVC job in
[run38468432446](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38468432446).
The other five jobs passed. The approved original CJK/emoji note's `WM_SETTEXT`
timed out after2,000ms, error1460. The UI stack was in RichEdit/GDI text metrics;
the recorded wait chain had no cycle. This recurs after the earlier `IMF_UIFONTS`
candidate, so that candidate did not establish a durable fix.

The unchanged parent test passed locally. The cold-host timeout is not locally
reproduced and the trace does not establish the particular font service or
size-adjustment operation responsible. The production correction below is a
bounded policy candidate; fresh exact-commit CI is required.

## Change and regression

The native plain-text report field retains its chosen UI font size across script
binding by clearing only `IMF_AUTOFONTSIZEADJUST` from its existing language
options. It retains `IMF_UIFONTS`, automatic fallback, IME/multicodepage handling,
accessible native editing, exact UTF-16/UTF-8 export and the surrounding font.
[Microsoft documents](https://learn.microsoft.com/en-us/windows/win32/controls/em-getlangoptions)
the default adjustment as scaling script-bound fonts, which a fixed-size report
field does not need. No font installation, DLL substitution, warm-up text,
sleep/retry, input suppression or deadline change is used.

The new native policy assertion failed before implementation on both form
lifetimes. This proves the undesired default, not the cold-host stall's cause.
The added native insertion uses Devanagari and Greek and checks exact exported
UTF-8. The original bulk CJK/emoji/CRLF note remains unchanged, along with consent,
private/oversized-input rejection, immutable retry, optional context and duplicate
export assertions. The two-second native message and30-second CTest limits remain.

## Local evidence

- Debug/Release apps and affected test executables built successfully.
- Debug report core/native UI2/2: UI1.76s, total4.38s.
- Release report core/native UI2/2: UI1.55s, total3.79s.
- The explicit2.2s injected stall still fails the two-second message deadline
  (error1460,2,003ms, expected exit1). It cannot silently pass this test.

Raw parent failure, red/green results and injected failure are under `logs/`.
No renderer, quality, saved graphics setting, asset or Android input changes.
Ordinary static-region saving remains zero and the chapter remains incomplete.
Fresh six-job CI and subsequent Unicode recurrence remain explicit gates; a
passing local UI test is not cold-host latency acceptance.
