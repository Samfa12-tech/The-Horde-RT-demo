# Windows separately consented local public review

Base02ece650. The existing Win32 local statistics form now offers an explicit
private/public review switch. It reuses the immutable completed summary, UTF-8
preview, scroll/keyboard focus behavior, Copy and user-selected Save path.
Selection clears approved bytes and resets consent, optional hardware/cooling,
preview, save state and export eligibility. Fresh preparation generates a new
CSPRNG report/result ID. Switching does not publish or grant send consent.

`WindowsBenchmarkReviewPayload` admits only the chosen typed private/public
payload and refuses replacement of prepared bytes until explicit new review.
The public path calls the existing sanitized typed builder; private wire shape
is unchanged. Results remain local, unverified and unranked, with honest absent
thermal/display-FPS evidence. No public send/receipt, account, service, website,
new data collection or gameplay/renderer path was introduced.

## Actual checks

- Windows Release diagnostic target and CPU report fixture compile successfully.
  The first compile caught a header included inside the `.inl` anonymous namespace;
  it was moved to the ordinary global include boundary and the corrected build
  passed. Original failed build log is retained in ignored reports.
- Two affected Release CTests passed in2.57s: typed benchmark summary/privacy/
  population/payload ownership and Windows source contracts. All preceding
  assertions remain; the source contract has40 checks. New CPU cases cover
  unprepared audiences, wrong typed admission, no consent inheritance, byte
  retirement, immutable exported copies and no prepared overwrite.
- Deterministic private fixture output SHA256 remains
  `6c0c4bdb67d0bab311f9f82cec63f9cc9f97972c4dd8b17b95831a6c5728617b`, identical
  to the previous Windows fixture including its output line ending. The public
  JSON payload is8664bytes before the command's line ending, with no private
  run/report IDs or remote/publish consent. These synthetic fixtures were parsed
  offline and never sent.

No native game/form, clipboard, file picker, foreground/controller, phone/ADB,
service or website execution was used. Actual Windows contrast/DPI/keyboard/
controller/picker and Android JNI/display acceptance remain pending. The installed
phone candidate is still e2addc77 and stopped at the verified09:27:50 release;
this source change has not been installed. Normal worker and other admission
gates are unchanged, with no new performance evidence.

[Exact remaining reporting interfaces/access/actions](../../design/HORDE_1_7_REPORTING_NEXT_HANDOFF_2026_10_28.md)
and [bounded read-only pre-fence assessment](../../design/HORDE_1_7_PRE_FENCE_FEASIBILITY_2026_10_28.md).
The latter is a feasibility/design finding, not a new runtime prototype or gain.
