# Preserve survey isolation ahead of benchmark recovery

Hosted head `471afc92` failed one of447 Android unit tests: `EnvironmentSurveyRequestTest.lifecycleAndInputReturnBeforeNormalHandlersAndAssetCacheIsShared`, line90. The other five required jobs passed; [the failed run](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40092430651) remains recorded in `retained-471-ci.json`.

This was a **source-ordering regression**, not stale fixture wording. New benchmark client retain/cancel/recovery handling preceded the isolated environment-survey exit; `onDestroy` also moved its guard beyond the unchanged420-character source gate.

The correction only reorders `MainActivity` callbacks. Survey Back/Pause/Stop/Destroy now returns before normal recovery work. Survey save-state uses the framework-only path before retaining any normal client. All normal benchmark recovery behavior remains after those guards. The test and its420-character window are unchanged; no assertion, coverage or timeout was removed.

Validation:

- Targeted survey/recovery: **33 tests, zero failures/errors**.
- Full Java suite: **447 reported**, zero failures/errors; **441 executed, six existing Windows symlink-capability skips**. These skips are not new passes or removed coverage.
- Lint: zero errors,80 warnings. Debug APK packaging passes. Java-only validation reused native outputs; there was no NDK rebuild.
- The actual corrected APK retains all six opening-benchmark JNI exports in each ABI, with all four native payload SHA256 values unchanged. This establishes export presence/reuse, not JNI invocation, Vulkan presentation or phone acceptance.
- APK206135428B, SHA256 `99f57593d782922e153d7839adc8f523db617d19504b96da3c1e6e008f18f419`.

Exact source/build/ordering/APK evidence is in `receipt.json`; symbol/payload reuse is in `native-payload-reuse.json`. Raw logs remain in ignored `reports/android-benchmark-survey-isolation-20261024/` and the preceding fixed-output report folder. All151 parked owner files still match their original hashes.

No phone, install, report transmission, foreground interaction, campaign/settings change or native rendering change occurred. Audio/haptic manual revalidation required for **this correction: NO**; the earlier benchmark transition listening gate remains pending. Corrected-head hosted CI must be verified separately.
