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

This lane detects selected CPU memory/undefined-behavior defects, not races,
Vulkan/device memory validity, hardware RT, phone performance or listening quality.
Audio/haptic manual revalidation:NO; CI only. No merge, signing or publication.
