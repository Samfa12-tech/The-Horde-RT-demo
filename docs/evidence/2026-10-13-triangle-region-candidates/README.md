# Triangle provenance and bounded GPU candidates

13 October 2026. Implementation parent `42d00817356b1bdd93b14a0684268152fb2d0795`.
The goal is active. **The chapter remains incomplete and normal-play residency
saving remains ZERO.** This is the next prerequisite for preserving graveyard
dressing during two-way loading, not permission to evict arbitrary tomb geometry.

## Why triangle ownership is needed

Consolidated bone, candle and urn material rows contain both indoor props and
outdoor grave offerings. Removing whole material families removes legitimate
exterior geometry. CPU-only provenance now survives placement and material
reordering, without changing the runtime shader ABI, row capacity, textures,
geometry, materials or admission budgets. All 6,184 graveyard triangles are tagged
exterior. The 13,516 tomb-dressing triangles include entry lintel offerings: the
label is source identity, **not proof of occlusion or permission to unload**.
Unclassified and masonry geometry is conservatively Shared and always retained.

An explicit Debug-only `--validate-static-triangle-cutover` selects Shared plus
exterior triangles, then restores the complete owner. It uses the existing GPU
upload, BLAS/TLAS, descriptor, current-pose, completion and retirement path.
Normal play still uses the complete owner. Partial diagnostic ownership cannot
publish normal zone readiness. New tests cover commit, rollback, scene move,
restoration and reconstruction of this contributor state; family-level diagnostic
omissions also fail traversal readiness closed.

Selection rejects malformed provenance, omitted-triangle bad indices, overlapping
or unaligned spans, missing records, overflow, aliasing and empty candidates
atomically. Retained triangle vertex attributes and order remain exact. Dedicated
player buffers, material/texture identities and unchanged BLAS-local numbering
remain unchanged. Cancellation and stale proofs preserve the admitted owner.

## Actual Windows replacement evidence

RTX5050 Laptop GPU, 960×540 at100%, Diagnostic/High, Higher shadows, frozen
`chapter-forest-lookout` camera. Both backend galleries were opened. Complete,
selected and restored frames have **zero changed pixels** within each run.
Town roofs/tower, terrain, raised lantern and player arms remain visible.
The untouched default portal is also pixel-identical to its parent on both backends.
This proves this view and actual resource transactions; it does not establish
all-angle visibility, secondary-ray/light irrelevance or ordinary-play streaming.

| Measurement | Value |
| --- | ---: |
| Selected-owner device-local reduction | 1,658,368 bytes |
| Selected-owner host-visible reduction | 824,576 bytes |
| Complete / selected owner allocations | 69,356,864 / 67,698,496 bytes |
| Complete / selected scene device-local bytes | 405,550,288 / 403,891,920 |
| Peak device-local overlap | 473,246,160 bytes |
| Primitive rows | 32 → 32 → 32 |
| TLAS owners | 1 → 2 during preparation → 1 |

The final Pipeline partial preparation took1,521.99ms, including **458.236ms
synchronous CPU packing**; cutover36.873ms. The phase-marked final Query preparation
took1,549.17ms, including488.272ms packing; partial cutover40.044ms. Its full
restoration cutover reached215.324ms. Earlier Query receipts remain preserved.
This is not acceptable
ordinary-frame preparation. Uploads are bounded across166 turns and actual AS
completion is polled; that does not hide the synchronous packing or cutover cost.
Full restoration uses the immutable cached CPU source across182 copy turns.

The narrow next step is incremental selection from that admitted packed source,
retaining per-triangle labels and fixed primitive/material identities. Contributor
eligibility must then be proved against actual opaque geometry, apertures, dynamic
lid state and all relevant ray/light origin domains. Clearance AABBs and one
unchanged frozen camera do not supply this proof. No partial normal-play commit
is enabled in this checkpoint.

![Complete — Pipeline](windows/pipeline/cutover-00-before.png)
![Selected — Pipeline](windows/pipeline/cutover-01-selected.png)
![Restored — QueryCompute](windows/query-final/cutover-02-restored.png)

Exact shader hashes, settings and owning GPU/resource measurements are in
the [Pipeline manifest](windows/pipeline/capture-manifest.json) and
[Query manifest](windows/query-final/capture-manifest.json), with transaction receipts
and pixel comparisons beside them. The earlier Query witness before the readiness
guard remains separately under `windows/query/`. Pipeline used the final common
code before added diagnostic boundary logs; Query used the final logged Debug EXE.

## Physical phone evidence and limits

The authorized SM-S948B separate Debug update to parent42d00817 was installed and
pulled back once. APK `24aaf0b5894e57a38804f39875c06fbf34b801dfd0c83997b76c8aea8fa9233e`;
unchanged development signer/versionCode10; Debug data preserved, release untouched.
The prior installed identity was retained rather than repeatedly queried.
Four admitted captures show the landing, lookout on both backends and visible Kit.
All screenshots were opened. These captures suppress voice; they do not prove
listening, acting, touch feel, full travel or final-candidate acceptance.

Two bounded lookout runs use the **original saved40% scale**, Mobile/Current,
576×1192 internal into1440×2980 swapchain, 1440×3120 screenshot. Each has120 warmup
and360 measured frames in three120-frame windows. The native checkpoint benchmark
**freezes camera AND simulation**: tick2516 and animation41.9162 remain fixed.
An earlier progress/helper label incorrectly called this live simulation; that
label is corrected, and the original states remain intact.

| Backend | CPU window means, ms | Final owning GPU sample, ms | Device-local bytes |
| --- | --- | ---: | ---: |
| Pipeline | 54.692, 54.716, 54.866 | 52.762655 | 166,076,608 |
| QueryCompute | 54.901, 54.790, 54.960 | 52.951457 | 166,076,096 |

These are roughly18 frames/sec for a frozen Diagnostic workload, **not a
performance pass**, per-frame medians/tails, Shipping performance or a full-route
measurement. Owning timestamp samples/resources are valid and finite; both thermal
status readings remain0. Cached and current-HAL temperatures are separately saved.
No resolution or quality was lowered. Both Debug runs stopped and original
preferences were restored before the owner disconnected the phone.
[Exact run receipts](android/sustained-receipt.json).

![Physical lookout — Pipeline](android/chapter-forest-lookout-RayTracingPipeline/frozen.png)
![Physical Kit — QueryCompute](android/chapter-kit-reunion-RayQueryCompute/frozen.png)

## Tests, artifacts and retained failures

Debug and Release each pass all six affected CTests: development static asset,
tomb dressing, graveyard dressing, forest dressing, initialization preflight and
static cutover. The new provenance RED test initially failed twice; the new
partial-readiness RED test failed three times. An early build selected a nonexistent
`horde_rt_scene_preflight_tests` target; the correct preflight target and all six
CTest registrations then executed. These failures are retained under `validation/`.

Android four native ABIs, assemble/lint and286 cached Java tests pass, zero
failures/errors/skips. Exact runtime/ASTC and held-item package contracts pass.
The original1,475-pose torch coverage and180-second policy remain unchanged;
unchanged torch/shader gates are retained from the parent and checked again by CI.
No shader, quality, capacity, assertion or deadline is relaxed.

Two native capture startup stalls remain **unattributed failures**: an early
Pipeline attempt stopped before capture began; a later Query attempt reached
scene-ready but produced no capture directory before the unchanged180-second
deadline. Failed stdout/stderr are preserved. Successful witnesses do not erase
these stalls or prove they are harmless. One phase-marked bounded Query follow-up
passed: host callback attachment, option parsing, directory creation, presented
partial owner and full restoration. Its selected/restored images were opened and
both have zero changed pixels. The intermittent stalls' cause remains unproved.

[Exact candidate/source receipt](candidate-receipt.json). Final common-code APK,
prepared and **not installed** while the phone is disconnected:
`f0729a6f2fe78526e369aa03d4268c2a1e8487da655d2af46496ed387e53a976`
(183,076,298 bytes), immutable local
`reports/phone-candidate-f0729a6f2fe7/horde-debug.apk`.
Windows executable hashes and changed source bytes are in the receipt.
Parent42d00817 passed all six CI jobs in run38240774167; this checkpoint's exact
commit CI must be reported separately after push. No audio/haptic manual
revalidation is newly triggered by these CPU/ownership changes; the full chapter's
still-pending owner listening and haptic review is preserved.

## Remaining journey gates

Normal-play two-way GPU loading/retirement and safe all-contributor selection;
incremental CPU preparation and measured cutover hitches; full moving route and
sustained Shipping performance; final Windows/phone lifecycle, complete route,
owner audio/haptic/touch and appearance acceptance. Persisted legacy Lower→Current
settings confirmation is still physically pending. The chapter remains incomplete.
Base conflicts remain; no merge, release or history rewrite. Easy Tree canopy and
mist issue28 remain deferred. Historical/untracked source and backups are preserved.

Next owner route: opening/guards → Keeper wind-up, death cue and mist clearing →
claim/rope → Kit and fresh lantern raise → all trail bends/lookout → return/descent →
second ascent. Check stable terrain/props, corpse/Kit state, equipment/light,
captions/voices, moon/shadows, and any traversal hitch. Phone pass also checks
Current/Higher, persisted Lower migration, orientation and background recovery.
