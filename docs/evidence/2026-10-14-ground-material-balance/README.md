# Ground material checkpoint

The terrain, trees and characters share ordinary opaque lighting and sRGB
decoding. The ground's lighter texture and upward-facing moon response explain
the broad contrast in the earlier attribution captures. This checkpoint changes
only `ChapterForestGround`'s linear base-colour factor to `[0.5, 0.5, 0.5, 1]`.
Trail sand, light strength/direction, shadow calculations, geometry/support,
roughness, normal/ORM maps, encoded texture arrays, quality and budgets remain
unchanged. This is a reversible development appearance default, not a measured
physical albedo or owner-approved final balance.

The generic named manifest override uses the existing PBR material field; it
does not add a shader branch, texture layer, sample or GPU allocation. Missing
overrides preserve imported values. The source-only
[recipe and untuned manifest](../../../assets/models/world/source/chapter-terrain-material-r01/README.md)
preserve the r21 GLB/Blender master and all six texture-array identities.

## Actual RT comparison

RTX5050 Laptop GPU, 960x540, 100% scale, High optical/water/fire, Higher shadows;
Debug Diagnostic, twelve settling frames. Both backends compare the same frozen
landing, second-bend and Kit views. Cameras, poses, light records, geometry OBJ
bytes, shader bundles, settings and resource counts match. The new untuned
landing/bend controls are byte-identical to the earlier attribution images.

- [Pipeline landing/bend before → after](gallery/ground-material-pipeline-comparison-r01.png)
- [QueryCompute three matched views](gallery/ground-material-query-comparison-r01.png)
- [Pipeline Kit clearing](gallery/kit-pipeline.png)
- [Tomb return and reset comparison](gallery/ground-material-interior-comparison-r01.png)

All twelve frozen images and all54 moving-route images were inspected, using
the labelled comparison/contact sheets. The trail remains readable; the broad
banks look less bright against the trees. The full native images and owning
records are retained under `native/` and `route/`; numerical comparisons are in
[image-review.json](image-review.json). Owner/phone appearance is still pending.

Each backend completes4,961 ticks/1,241 moving RT frames: landing360, fresh raise,
Kit walking/waiting, bends/lookout360, return/descent, lower-room sweep, repeat
ascent and reset. This begins after staged Keeper death/first ascent; it does
not certify fresh combat or owner play. Traversal/equipment/actor records and
resource counts match the prior texture-lifetime route. The fresh opening is
pixel-identical. Lower-room frames differ in2–117 pixels, by at most2 display
units. The selected lower-floor crops differ in0–5 pixels, by at most1 unit.
No changed floor-shadow pattern is visible; byte-identical floor output is not
claimed for those views, and the tiny differences are not separately attributed.

## Validation and retained failures

Debug/Release applications build and each passes13 focused CTests, including
all seven requested traversal tests, native importer/negative argument tests,
terrain correspondence/exclusion and resource cutover. Android four-ABI Debug
build/lint, fresh286 Java tests across45 suites (zero failure/error/skip), actual
package/held-item bytes and unchanged development-certificate verification pass.
Fresh eight-key shader compilation matches the catalog and unchanged limits.

The new colour override first fails four behavioral assertions before parser
implementation. The package check then rejects the old manifest pin; its new
exact781-byte/hash pin passes strict positive/negative package policy checks.
The full Debug torch gate times out twice at the unchanged180-second deadline.
Its repeated118-triangle scans are replaced by a conservative per-pose sample-
cloud broad phase. All original points, triangle tests, pose/skin checks and
limits remain;944 exact full-scan comparisons and empty/nonfinite fallback
checks are added. The final gate passes in158.07s Debug and13.74s Release, with
the same3,287 poses,1,812 full viewmodel skins,3,178 plane and57,970 disk samples,
0.0323757m worst clearance and0.0118211m maximum grip error. Candidate triangle
visits fall from1,874,944,244 to15,135,254. This reduces verification work, not
production GPU cost. The first filtered run also timed out; host variability
is not fully attributed. Bounded progress/stage records now expose slow runs.
All failed logs remain under `logs/`; no deadline/assertion/coverage is weakened.

[Exact source/build/APK identities](source-and-artifact-receipt.json) bind
pre-commit builds to base735f2845 plus actual input hashes and canonical Git
blobs. Prepared uninstalled APK SHA-256:
`1a00b34d61ce63423748c5b05434ae1a7744cbb55d196c6be5fa40e82089be6b`,
183,170,474 bytes, versionCode10, unchanged development certificate. Only four
native libraries, the terrain manifest and attribution file differ from the
preceding prepared APK; Java and the entry roster are unchanged. No phone action
occurred. Exact pushed-commit CI is verified and linked on PR27 after the push.

## Next completion gate

The chapter remains incomplete. **Ordinary two-way static-region streaming is
the next milestone**, before unrelated polish or optimization. Detailed regions
must prepare before use, publish safely, retire after completed work, and reload
on return while retaining coarse land, visible distant landmarks and every
relevant light/secondary-ray contributor. Static-region residency saving remains
**ZERO**; the existing32 MiB Windows Keeper-only saving does not close this gate.

Use a representative optimized Release out-and-back route at unchanged quality
to measure transition overlap, memory before/during/after, sustained frame times
and hitches. Debug CPU and GPU-only timings are not playable FPS. Collision,
shadows, reflections, stale/failure readiness and repeated traversal must pass.
Phone route/Current/Higher/Lower-migration and owner visual/audio/haptic acceptance
remain pending. Easy Tree/canopy and mist28 remain deferred. Audio/haptic manual
revalidation for this material/test slice: **NO**; prior chapter cue acceptance
is still open. No merge/release.

Owner route: tomb/guards → Keeper/claim → rope → fresh lantern raise/Kit → both
bends/lookout → return/descent → second ascent. Judge the bank/tree balance,
trail visibility, equipment/light ownership and any loading pop or hitch.
