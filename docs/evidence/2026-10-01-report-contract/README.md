# Unwired player-report contract checkpoint

Historical checkpoint below is preserved. Native forms/local export subsequently
integrate it at3e0f9be/69afe4e; see [new evidence](../2026-10-01-report-export/README.md).
No remote-delivery acceptance follows from that later local integration.

Typed cross-platform schema1 builder and foreground delivery-state seam only;
**reporting feature not complete**. No platform form/export/remote transport,
endpoint, automatic upload, background retry, screenshot/log/save attachment or
backend configuration. Continued platform work is deferred by the owner's staged
RT experiment steering, not discarded. Briarhold source inspected at
`src/playtest-reporter.js`, `src/playtest-diagnostics.js` and
`tools/playtest-report-store.mjs`; game-specific JS/server code is not copied.

Explicit submission/export consent and separate basic-context opt-in default off.
4096-byte UTF-8 player note,512-byte context fields,16KiB final JSON cap, safe
opaque ID/UTC/enums and typed build/platform/model/renderer allowlist. Known
credential/contact/URL/private-path patterns reject preparation before output;
not comprehensive secret detection or server-side validation. No silent truncation
or source-note mutation. Only explicit retry retains the same prepared ID/bytes;
cancel invalidates late attempt completion. No real delivery/dedupe claim follows.
Prepared records are trusted builder outputs, not a general external JSON parser.
Strict note/context restrictions need platform usability review before integration.

Initial focused tests exposed unsafe string_view test backing and embedded drive-
path detection. Lead review also found UTF-8 continuation/control confusion.
Fixes retain owning fixture strings, decode codepoints, detect embedded drives and
reject blank notes/forged oversize attempts. Regression coverage accepts Chinese/
emoji, tests final JSON cap, defaults, enum/time failures, privacy, retry and cancel.

Fresh MSVC Debug/Release focused1/1 PASS (2.47s/1.08s); lead ASan1/1 PASS1.28s,
no diagnostic. Logs retain initial failure and corrected builds/tests. Shared
source roster compiles the unwired unit on Android; latest FPS Debug build/lint
also passes, not a reporting-device pass. Follow-up: local user-selected export
forms/context/cancel/offline UX, then an explicitly authorised backend/real-delivery
gate. Do not borrow Briarhold's endpoint or send unexpected test reports.

Audio/haptic manual revalidation required:NO; this unit is unwired and has no
gameplay, feedback, playback or renderer authority.
