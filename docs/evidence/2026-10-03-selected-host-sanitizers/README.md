# Audit F10 — selected ASan/UBSan host lane

Parentaa2000b. The existing GCC/Clang/MSVC, focused Vulkan CPU-host and Android
lanes remain intact. An additive Ubuntu24.04 job runs the finite15-target roster
in `tools/run-host-sanitizer-validation.sh`: shared simulation, animation/IK/grip,
physical math, Core/Horde PCM decoder/cursor/asset-bank/session, mailbox and
consent-bounded report/submission contracts. It fetches only their fixture and
PCM payloads. No shared Core source, production build preset or runtime changes.

Clang compile and executable link use ASan/UBSan; compile also usesO1,debug
symbols/frame pointers and non-recovering diagnostics. Runtime leak detection
and halt-on-error are explicit. Each required CTest registration runs by exact
name with `--no-tests=error`, so a missing registration fails rather than shrinking
the passing subset. No automatic retry, suppression file or test timeout change.
References: [ASan usage](https://clang.llvm.org/docs/AddressSanitizer.html#usage),
[UBSan usage](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#usage).

Local Bash syntax check, workflow YAML parse and diff whitespace PASS. The default
Python lacks PyYAML; an isolated parser6.0.2 atC:/Dev/tmp/horde-ci-yaml-check
validates the workflow without changing project/runtime dependencies. Local
Linux Clang/compiler-rt is unavailable (WSL not installed); **no local sanitizer
execution is claimed**. Current-source GitHub execution is the acceptance gate,
not the presence of this job or passing unsanitized tests. Its exact handles and
results must be retained after the coherent push; do not rerun older jobs.

## First current-source execution — f2ebe42

Push [37068625638](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37068625638)
and PR [37068630417](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37068630417)
both complete with the sanitizer job SUCCESS. The PR job111042520058 log confirms
Clang18.1.3 and all15 named registrations passing individually with no sanitizer
diagnostic. The focused Vulkan lane passes16/16, including the new driver-report
contract, and Android builds/lints with76 Java tests, zero failures/errors/skips.

The complete workflows are **FAILURE**, not green: GCC/Clang each pass58/59 and
MSVC64/65. All fail the same player package inventory assertion. Its old regex
consumed every job between shared-gameplay and player-vulkan-host, including the
new sanitizer lane, falsely doubling the viewmodel fetch/checkout entries.
The bounded correction stops at the next sibling job. LF/CRLF inserted-sibling
positives, missing-checkout and missing-job negatives retain the admission gate.
Fresh current-source CI is required after that correction; no old run is retried.
Logs are retained atC:/Dev/tmp/horde-f2ebe42-ci/pr-{clang,gcc,msvc,vulkan,android}.log
and pr-sanitizers-job.log. Do not repeat the completed diagnostic investigation.

This lane detects selected CPU memory/undefined-behavior defects, not races,
Vulkan/device memory validity, hardware RT, phone performance or listening quality.
Audio/haptic manual revalidation:NO; CI only. No merge, signing or publication.
