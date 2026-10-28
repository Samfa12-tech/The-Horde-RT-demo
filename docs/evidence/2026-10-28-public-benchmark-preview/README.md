# Separately consented public benchmark preview

Based on c1a4d16d. This is a local typed projection and source/component checkpoint,
not a public submission, website, backend, email or native/device acceptance. No
phone/ADB, game launch, CUA, controller/foreground use or service mutation occurred.

`PublicBenchmarkReport.h` exposes only frozen-evidence preparation with independent
local public-preview consent, optional hardware consent and a fresh result UUID.
It cannot accept private JSON or a prepared private report, and the private-send
factory does not accept its type. A run UUID cannot be reused as a public ID.
Future platform publication consent and compatible server transport remain separate.

Projection omits full timestamp, private run/report identity, raw rows/owner IDs,
private legacy metadata, notes/logs/paths and tokens. Optional reviewed model/GPU
labels are absent unless expressly selected. A coarse date, actual configuration,
aggregate CPU/GPU/status populations and limitations survive. Results remain
community/client-reported, unverified and custom/unranked. Unavailable version,
OS/driver, output-policy and packaged-artifact fields remain null rather than
invented. The [API handoff](../../design/HORDE_1_7_REPORTING_API_2026_10_27.md)
records the concrete local allowlist and remaining service/platform steps.

Three affected Release CTests passed in3.24s: benchmark summary (including the new
measured-owner public cases), compatibility builder and Windows private submission
owner. They cover independent consent, omitted/unsafe optional labels, invalid
calendar/identity/incomplete data, actual submitted pixels/DRS/populations,
declared-versus-measured cooling and immutable separately approved retry/reset.
No previous assertions/deadlines or platform transport gates were removed.

An independent JSON parse checks exact top-level/result/hardware allowlists,
unique keys, finite JSON, coarse date, omitted private values, absent verification/
ranking, and equality of configuration/aggregate/zonal data with the same measured
private fixture. The public fixture is8665bytes (limit16384). The private wire
fixture is byte-identical before/after extracting the common configuration writer:
SHA256 `6c0c4bdb67d0bab311f9f82cec63f9cc9f97972c4dd8b17b95831a6c5728617b`.
Fixtures are synthetic local measurements and were not transmitted or published.

Initial sandboxed MSBuild regenerated successfully but then made no logged
compilation progress for nearly three minutes. That command was explicitly
interrupted (exit1), then the same affected build through ordinary approved host
access completed (exit0). Cause is not established; no test failure, protection
change or blocked game launch is inferred. The nonessential process CIM query
was denied and not retried. Raw local receipts stay ignored under
`reports/public-preview-*`, not duplicated as committed archives.

Android JNI native compilation passed for arm64-v8a, armeabi-v7a, x86 and x86_64
in2m51s. New exact-head CI still needs its own result in the checkpoint handoff. No Java/UI changes were made, so this slice does not repeat
or claim new physical touch/SAF/rotation evidence. Windows appearance and real
export are similarly separate. The website checkout denial is unchanged; no
alternate-path or replacement-checkout workaround. Normal flask/rings/yellow/
shared64 remain OFF. Preserve all151 parked owner files and saves/settings.
