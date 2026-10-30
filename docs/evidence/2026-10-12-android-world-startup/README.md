# Android world startup and exact-intake correction

The actual SM-S948B stopped at native startup because the grave-marker manifest
was packaged but absent from the app's filesystem. The same staging omission
affected eighteen graveyard, rock, fern, deadwood and town payloads. Startup now
stages the complete dependency set before publishing the native report root.
`WorldAssetStagingTest` compares the independently packaged world roster against
exact, once-only startup destinations. It failed before implementation and passes
afterwards. No release app, signature, version code or saved data was cleared.

![Missing dependency before repair](phone-missing-manifest.png)
![Actual clean native start after repair](phone-clean-start.png)

The clean start has three hearts before movement or damage. This does not explain
every earlier two-heart observation. The screenshot and app-scoped presentation
log establish a real Pipeline frame; they do not certify route, audio or touch feel.
The intermediate APK identity and limitations are in [receipt.json](receipt.json).
At unchanged requested 100% scale, 29 bounded valid GPU samples averaged **486.995
ms**. This is poor performance, not sustained acceptance. No scale, quality or
shader limit was reduced to hide it. Normal-play GPU residency saving is **zero**.

CI `38204512666` on `260aeb75` exposed an outdated integrated intake count. A fresh
local build reproduced the same failure: 31 primitives/30 materials stayed within
the existing 32/32 caps, but the expected indices omitted **11,884** newly admitted
deadwood triangles. The expectation now includes these actual geometry bytes;
failed intake prints each pre-existing geometry invariant. Red/green logs are
retained; no capacity or assertion was removed.

The exact MSVC step metadata shows 7m15s building and 6m37s running CTest, followed
by cancellation at the unchanged 15-minute job deadline. All 138 CTests passed;
later offline checks did not all finish. The asset-policy fixture alone used
222.43s. The revised Windows runner overlaps only that repository-read-only,
GUID-temporary-directory fixture with the otherwise sequential selection. It
checks the complete selection first, selects the fixture exactly once and fails
if either process fails. The same exclusions, assertions and deadlines remain.
Local scoped execution passed the complete asset-policy fixture plus cutover and
resource-inventory tests; all 182 asset cases remain. A first local attempt used
an unsupported CTest noncapturing regex; it was corrected to ordinary grouping,
and its failed evidence remains under `reports/windows-isolated-policy-overlap-invalid-regex.log`.

Validation: all four Android Debug ABIs, lint and **285 Java tests** passed;
runtime/ASTC/hash package and held-item checks passed; fresh Release intake passed;
the isolated Windows overlap check passed. Exact new-commit CI remains required.
The unused moving-frame static-owner API is separate ongoing work, not adopted
streaming or proof of any residency saving. Final two-way route, controlled
sustained measurements, Current/Higher and saved Lower migration, owner audio/
haptics and the final identified phone artifact remain acceptance gates.
