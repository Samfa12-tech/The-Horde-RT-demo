# Survey export API 24 compatibility - 22 October 2026

Hosted Android lint reports 16 NewApi errors: EnvironmentSurveyExport uses
API 26 Files/toPath calls while the app supports API 24. This is a real platform
compatibility defect, separate from the checkout/fixture failures.
[Retained hosted excerpt](hosted-lint-baseline.txt).

The replacement uses API 21 Os.lstat and one atomic
Os.open(O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0666), preserving the existing
exclusive-create behavior. Canonical-parent checks, rejection of symbolic links,
byte bounds, original payload custody and failed-export evidence remain.
Only ENOENT means absent; other path-inspection errors propagate. The output
stream owns and closes its descriptor explicitly, including wrapping failure.
No minSdk, dependency, data location, gameplay/save or permission change.

## Actual validation and limitations

Local `:app:testDebugUnitTest :app:lintDebug --max-workers=1` succeeds.
[Build output](android-checks.txt), [all suite counts and lint IDs](android-results.json).
The result XML contains **405 passed, 6 skipped, zero failures/errors** across
411 cases. Lint has **zero errors/fatal issues and 75 warnings**; this is not a
claim of a warning-free application.

The focused export suite runs at API 24 and 34: **22 passed, 6 skipped**.
Existing export/corrupt/missing/bounded-content assertions remain. New checks
cover exclusive-create sentinel bytes, descriptor closure, requested open flags,
non-ENOENT lstat errors and modeled symbolic-source rejection.
[Individual cases](export-cases.json).

Installed Robolectric 4.14's default Linux/Posix lstat follows links and its open
shadow ignores flags. The focused suite therefore supplies an explicitly
**test-only syscall contract model**, asserting the production flags and
no-follow metadata. It does not certify actual Android syscalls or filesystem
race behavior. Six actual host-link cases (three cases at each API) are skipped
on this Windows host; no pass is claimed for those. The host model's split
create/wrap is not used in production, whose exclusive open stays atomic.

No phone operation occurred. Actual Android export/link/descriptor behavior
remains physical-device evidence pending a future owner-authorized connection.
No audio path changed, so this checkpoint adds no listening requirement beyond
the already-open Kit endpoint/mix acceptance. PR conflicts, Debug route timeout,
natural gameplay/recovery and warmed phone performance remain open.

Next inspect the backed-up exact-head hosted rerun, including Linux link cases
and all Android ABIs. Do not weaken any gate to match a pass.
