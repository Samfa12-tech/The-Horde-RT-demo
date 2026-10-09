# Verified 1.6.2 publication — 9 October 2026

The completed standalone tomb goal was followed by the owner-authorized merge of [PR18](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/18), stable-key Android signing and publication to both [itch channels](https://samfa12.itch.io/the-horde). Both channels report completed version **1.6.2**. Fresh downloads match the signed Android APK byte-for-byte and every one of the 96 Windows payload files. This is actual distribution verification, not upload-success inference.

Validated runtime/build source: `db62032d9ab54ebaa5bd12d17dc7e987fc8f54c9`. PR18 merged to main at `a3f120cf1f3995035f793fa40400e0a74a4c0459`, 2026-10-09T00:09:35Z. Runtime/build inputs remain identical to the validated source; promotion changes signature metadata and Windows distribution text only. PR18 head `ab97f503` passed aggregate 12/12 CI jobs (PR run 37862273391; push run 37862269477). Later publication bookkeeping has separate CI.

| Published artifact | SHA-256 | Bytes / channel |
| --- | --- | --- |
| Signed Android APK | `cc4de768cf9ca2a409984a24888399e9c518363c359b3a9947d1ed0350c254e8` | 121,071,933; Android build 2090804 |
| Windows publication ZIP | `18177d0d84c40f0831b22ee959be597deeb3bc0e1cef183e78dd19c1b113f21c` | 145,574,438; windows-x64 build 2090803 |
| Windows Release executable | `084e4a345b556cdf6c3e8b4adbaef488c22cee02d41226dc43c49c5cf873260d` | Included in the verified 96-file Windows payload |

Android versionCode is **10**. Stable signing certificate SHA-256 is `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`. The in-place signed Shipping install on SM-S948B / Android 16 was pulled back to the exact APK hash; normal Entry → Play → Pause → native diagnostics confirms actual RT presentation. No data was cleared or settings controls changed. Shipping private preferences cannot be read, so this is not a byte-identical production-preference claim. The earlier exact Debug preference evidence remains separate.

The exact packaged Windows Release executable presented real Pipeline RT on the RTX 5050 Laptop GPU after the owner clicked the window. The prior unfocused ordinary Release timeout and failed Debug moving arming attempt remain failures of those scoped attempts. This successful normal Release startup does not retroactively close the Debug moving or secondary-view coverage limits. Both owned apps stopped.

The [machine-readable receipt](receipt.json) records stable channel identities before/after fetch and exact payload checks at 2026-10-09T00:19:54Z. Butler distributes the Windows directory payload; the generated itch download archive is not claimed byte-identical to the local publication ZIP. Android upload 18341739 / build 2090804 and Windows upload 18339908 / build 2090803 were completed and unchanged across download.

Fresh/reset phone defaults remain **50%**, Mobile water/fire, Glass Off, Current shadows, cap30, Mist On and Dust Low. Desktop remains 100% / High water/fire / Glass On with the other defaults unchanged. Saved/custom choices remain authoritative;33%/40% are experimental. The [final technical review packet](../../ENGINEERING_1_6_2_FINAL_REVIEW_2026_10_09.md) preserves exact Host, Debug-device, static-review and owner acceptance evidence.

Current forgiving combat, the minor walking torch-arm wiggle, shafts, seamless music handover and long sustained-phone performance work are deferred. No sustained 30 FPS or untested-device/controller certification is made. Eric's independent audit packet is prepared; his audit is not self-certified. Owner signing backup/recovery checks remain owner-only and unchecked. No GitHub tag or GitHub Release was created.

The pre-release review receipts and sealed unsigned artifacts retain their original state. This separate post-completion receipt supersedes their then-true “not merged/signed/published” status without rewriting historical evidence. The published 1.6.2/code10 line is now immutable; a future production package requires a new source version and higher Android code.

Four affected local policy checks pass in21.50seconds: native version contract, PowerShell7 and built-in Windows PowerShell release policy, and GitHub release preflight. They prove the newly frozen line and suffixed variants reject before builds, key access or upload lookup. This bookkeeping does not rebuild or alter the released runtime.
