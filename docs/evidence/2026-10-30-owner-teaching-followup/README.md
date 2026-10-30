# Owner acceptance and sword-ready teaching follow-up

## Owner observations, frozen R8

Sam took over the isolated Windows acceptance window after physically stopping
Computer Use. On 30 October he reported that everything seemed to work as
intended, explicitly confirming Slot 3 loaded, save/load worked, the real menu
cursor was visible, and Alt+Tab away/back worked. This supplies the native
cursor observation that the earlier Computer Use overlay could not establish.
It is owner-reported keyboard/mouse play evidence, distinct from the earlier
synthetic checkpoint/ADB tests. Do not invent a separately recorded combat,
death/retry, endpoint, controller or Android sequence from that brief report.

The tested R8 SHA256 remains
`38144dd9d9176b2d4af16e9bf0e12034452438332231ff55eab803669c5c4e85`.
Its original source/provenance is retained in the
[device receipt](../2026-10-30-three-slot-device/README.md).
The published-download intermittent cursor report is not a patched binary or
a reproduced root cause; the current R8 owner observation is positive.

Two exceptions were reported: Keeper's "I sense you" and "Come closer" were
not heard, and the Q/block lesson appeared at game start before drawing the
sword. These remain separate findings.

## Corrected lesson eligibility

The shared immutable teaching snapshot now marks the lesson suppressed until
`SwordDefenseReady()` is true. Windows and Android hide the practice label and
lesson while the sword is stowed/drawing or otherwise unavailable. They reveal
the existing input-aware lesson when defense is ready, and clear a stale
practice fade when readiness is lost. This is presentation only: practice
protection, learned skills, input, attack rules, timing and accepted guard/attack
animations are unchanged. Authoritative attack warnings and revival-clearance
messages remain independent. Android carries the flag in previously unused
packed bit 35, without changing saved data.

Native tests exercise the actual stowed-to-drawn simulation transition, retained
protection/progress, and independent red warning. Android tests exercise touch
and controller lesson text and the packed suppression bit. Five affected CTests
pass (combat foundation, chapter dialogue, Windows combat presentation, subtitle
bitmap, desktop controller). Windows Release application builds. Full Android
Java: 536 registered, 530 passed, six unchanged host-capability skips, zero
failures/errors. Lint: zero errors / 92 existing warnings. All four Debug ABIs
assemble successfully. No assertion or deadline was relaxed.

## Keeper audio diagnosis is still open

Both packaged WAVs match source bytes, decode as non-silent 48 kHz mono PCM16,
and have the expected 2.590 / 2.868479-second durations. Existing private audio
logs do not show a missing-asset/decode failure, but do not record each chapter
voice admission. A new current-rule CPU witness walks from the torch trigger to
the Keeper threshold and observes both dialogue identities with nonzero spatial
gains. It uses a deliberate starting position, not a natural chapter playthrough
or physical listening test. An initial invalid fixture starting past the torch
trigger never began the reveal; that failure was corrected, not called a game
defect or weakened away.

No gain, clip, reveal timing or story rule was guessed to fix the unheard voice.
Windows now records bounded per-generation playback admission/outcome and its
applied spatial/voice gains in the existing private audio log. Repeated failed
frames do not spam entries; a later successful admission is recorded. Engine
admission is not proof that the owner heard the clip. The next real encounter
can distinguish missing trigger from playback failure or an inaudible mix.

## Prepared, not presented replacements

Separate R9 Windows package, 184 byte-verified existing assets, fresh isolated
state: SHA256
`9308472aebdd6a569fbf668670b3f0deb7403789d15649fe28c1bccb314fe3e9`.
Prepared Android Debug APK, all four ABIs: SHA256
`e27e37f944730ed461954d2b0fb18fa74bf8f70e4d359e390e0cdfa933dd8ee2`.
Their embedded base is `58a27a73` plus the bounded dirty input now backed by this
source checkpoint, not a falsely clean build of a later receipt. Full per-ABI
labels/hashes are retained privately under ignored
`reports/keeper-teaching-feedback-20261030/`.

Neither replacement was launched/installed. Sam's running R8 window and saves
were left untouched. The phone was withdrawn at 10:19 UTC; no phone calls,
installation or foreground automation followed. Existing installed Debug,
production and actor-worker packages were not changed. Runtime visual review of
the readiness correction and actual Keeper listening remain pending. Benchmark
entry/return/cancel/settings/export and other documented merge/release gates
are not closed by this source follow-up. No merge or publication occurred.
