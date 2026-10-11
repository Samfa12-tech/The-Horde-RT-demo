# Explicit immutable chest material sharing

Parent: `6124e51ce12cd4b9c051a5824633448f9f51bd16`. This commit's source
produced the exact artifacts and shader identities in [receipt.json](receipt.json).

The development chapter now explicitly shares the chest lid's two immutable,
opaque material records with the base. Every converted GPU field must match.
All primitive references are rebased atomically before upload; geometry,
articulation, sockets, texture bytes and mutable dielectric ownership are retained.
Invalid, unequal, chained, duplicate or forward aliases fail before publication.

Static material rows fall from 30 to 28 (3,840 to 3,584 uploaded bytes), leaving
two rows for the authored terrain within the existing 32-row total including
the two world-palette rows. This is capacity savings, not a measured FPS gain.
All instance, primitive, texture, shader and quality limits remain unchanged.
Terrain and its matching authoritative support are still not live in this slice.

## Verification

- Debug affected CTests: 2/2, 11.28 s. Release: 2/2, 2.58 s.
- Android Debug build, Java unit tests and lint: PASS, 59 tasks, 1m26s.
- The first compile failure and intentional integration red results are retained
  locally. The additional stale count assertion was updated to require the exact
  reduced count and actual chest row identities, with the mutable fixture distinct.
- Native frozen reward checkpoint 115, 960x540, scale 1, Diagnostic High:
  [Pipeline](Pipeline.png) and [QueryCompute](QueryCompute.png) complete and inspected.
  Each backend's pixels are byte-identical to its earlier rigid-grouping capture.
  The existing cross-backend difference remains 94 pixels, maximum channel delta
  30; this is not a new claim of cross-backend pixel identity.
- The first Pipeline capture crashed with access violation `0xc0000005` in
  `ntdll.dll` before a frame was saved. A fresh isolated retry passed. Private
  logs and the crash dump remain preserved. Stack-address inspection is not an
  unwound call stack and does not establish cause; this intermittent failure
  remains unattributed, not fixed by a retry.
- Shader sources/generated code are unchanged. Push CI is now enabled on the
  current development branch so PR base conflicts do not suppress compiler gates.

## Phone and remaining gates

Read-only checks confirm an authorized SM-S948B with the earlier Debug APK
`1bd0611a…4886c`, version 1.6.2-debug / code 10. Installed and candidate signing
certificates match. No install, launch, input or capture was performed on the
phone. That old APK provides no acceptance evidence for these changes.

Chapter terrain/woods, visible Kit and actual resource handoffs remain incomplete.
Phone installation approval and fresh physical/backend/lifecycle, owner visual
and audio/haptic acceptance remain separate. No release or merge.
