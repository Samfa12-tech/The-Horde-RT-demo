# Bounded Android graphics UI acceptance - 20 October 2026

SM-S948B / Android 17 API 37, normal entry menu, portrait 1440x3120 with the owner's
existing font scale 1.7 and density 560. Same existing WP2 worktree, branch and draft
PR27. This is noncombat UI/persistence evidence, not a performance experiment.
[Exact APK/source hashes, scopes and custody](receipt.json).

## Actual results

| Check | Result |
| --- | --- |
| Compatible update | Both Debug updates used `adb install -r`, same package, versionCode 10 and verified signing certificate. Installed remote APK hashes match. Before launch, first update preserved settings and all 12 campaign records/locks byte-for-byte. No uninstall, clearing, permission grants or security changes. |
| Draft / Restore | Staged Mist Off while Saved/effective remained On; settings stayed byte-exact. Restore returned On with a matching current-generation RT acknowledgement. |
| Single Apply | On ed44d417 APK e62249..., one Apply saved Mist Off at 40%; requested/Saved/effective tuples match, real RayTracingPipeline presentation, serial 3/generation 1 state 2 acknowledgement. No Keep action exists. [Actual screen](mist-off-saved.png), [UI details](mist-off-saved.xml), [graphics log](original-apk-graphics-log.txt). |
| Outstanding Apply / Home | One 100% Apply (serial 4/generation 1) followed immediately by Home left Saved 40% / Mist Off and pending=true / candidate 100. That request has no save acknowledgement. Hot resume brought the same task forward; when Graphics was reopened, current RT was 40% / Mist Off and pending=false. Candidate 100 remained retained diagnostic metadata until the original tuple was reapplied. [Interrupted custody](interrupted-apply-custody.json), [recovered custody](recovered-custody.json), [recovered UI](recovered-graphics.xml). |
| Discovered label defect | Original APK displayed Confirmed and saved for a later unsaved resolution draft. [Preserved screen](old-stale-saved-label.png) and [UI details](old-stale-saved-label.xml). This was a status-text error; the Saved values and on-disk preferences remained unchanged. |
| Corrected exact APK | c5a11b... checks equality of draft and Saved before showing Confirmed and saved. One Apply of the restored 40% owner tuple reached state 2/current RT, then a 50% draft correctly showed Not saved while Saved/effective stayed 40%. Restore and Back pass. [Corrected actual screen](corrected-unsaved-draft.png), [UI details](corrected-unsaved-draft.xml), [graphics log](corrected-apk-graphics-log.txt). All 370 Java checks and four-ABI Debug build pass. |

The second APK changes only that UI predicate and its regression. Home interruption
was not repeated on it; preserve the two exact-artifact scopes above rather than
claiming the entire matrix was rerun. PNGs are original device screenshots. XML
snapshots occur after their initial screenshots, so transient labels may advance
between them. Full raw observations remain under ignored
`reports/graphics-apply-phone-2026-10-20/` in this worktree.

## Owner custody and remaining gates

Original and final graphics are 40%, Mobile water/fire, cap 30, Glass Off, Current
shadows, Mist On and Dust Low. The explicit original Mist On choice was restored as
requested; Android fresh/reset defaults remain Mist Off. All preference names,
types and values match exactly. Raw XML differs only by one trailing blank line:
[before](before-settings.xml) SHA e3966b... versus [after](after-settings.xml)
SHA 59fe30.... Preference entries are identical; only trailing whitespace differs.
All 12 campaign files/locks are byte-exact, the production save directory remains
empty, and the test-launched Debug process is stopped. [Before custody](before-custody.json)
and [final custody](after-custody.json) retain the exact hashes.

No resource/storage failure was forced on the phone. Those rejection/fallback
branches have injected host coverage; this run proves Home cancellation/recovery,
not a physical failed-allocation or failed-disk test. Controller, rotation, other
font scales and preview/Image interaction remain separate gates. Two initial
launcher command errors were rejected before launch, corrected after Android
resolved its actual component; no approval denial was bypassed.

The next approved critical-path milestone remains WP2a visible contact and natural
earned lantern/checkpoint/death recovery, followed by full rope out/back acceptance.
The feature inventory and GameSimulationRescue already contain the production
progression; another save architecture or diagnostic loop is not the next need.
The earlier denied combat action and Keeper preparation/arm/timing sequence remain
stopped. No gameplay retry, new heavy outdoor content, public push/release or Reef
operation occurred during this checkpoint.

Audio/haptic manual revalidation required: NO. Only a graphics status predicate
changed; gameplay feedback, event timing, cues and gains are unchanged.
