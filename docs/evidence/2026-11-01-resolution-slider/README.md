# Normal resolution slider, 1 November 2026

The owner requested 25% as the normal minimum and removal of the separate
33%/40% experimental buttons. Windows trackbar and Android slider now offer
25–100% continuously; ordinary labels contain only the percentage. Preview
presets include 25%, with the exact current saved value included when needed.
Fresh Android 33% and Windows 100% defaults are unchanged. Saved 33%/40% and all
other valid percentages remain valid. Apply/actual-RT acknowledgement/Keep,
revert and interrupted pending-choice safeguards remain in force.

Shared validation, rounded internal extent, persistence/recovery and benchmark
summary admission agree on 25–100%. Native output extent is unchanged. Legacy
isolated benchmark 33%/50% admission and the private 20% outdoor probe retain their
explicit restrictions; this does not reopen those experiments.

## Validation

Windows Release app compiled successfully. Six selected native/policy tests passed:
settings, normal 25% admission, legacy 33%/50% admission, benchmark summary report
and CMake scale policy. Coverage includes all slider positions, controller steps,
25% rounded extents for both hardware RT backends, matching presented tuple before
Keep, confirmed recovery, stale/wrong acknowledgement rejection and exact report
extent validation. Android offline focused tests passed 54/54 (21 preferences, 28 preview options,
5 compact layouts), with no failures/errors/skips. Final assembleDebug passed
for arm64-v8a, armeabi-v7a, x86 and x86_64. The final APK SHA256 is
`2b38d60394e5278f7ef40345236b824d4d70466e60376df7e6a1a62217d08a2a`.
Device interaction is recorded separately by the lead agent.

The first test run exposed obsolete experimental-label/range expectations and a
fixture that changed the draft before validating the currently presented tuple.
The tests were updated to the new normal range while retaining transaction,
presentation, invalid-value and saved-preference assertions. No assertion was
relaxed to accept an unmatched RT acknowledgement.
No performance claim, public release or merge is implied. Physical Windows
controller/mouse acceptance remains separate from host slider mapping tests.

## S26 device check

Actual normal settings UI selected and saved 25%, with matching RT effective
extent 360 x 743 inside unchanged 1440 x 2971 output. After force-stop and
relaunch, saved and effective values remained 25%. The original 33% preference
and all six original preference/career files were restored and hash-verified.
Latest debug remains installed and stopped; phone released. No performance or
human controller/touch-feel claim. [Device receipt](ANDROID.json).
