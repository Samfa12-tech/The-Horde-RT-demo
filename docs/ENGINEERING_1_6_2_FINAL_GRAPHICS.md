# Final graphics defaults and mist control

**Integrated stand/default checkpoint — 9 October:** source `ac97da91` adds shared physical collision for the existing two Keeper torch stands, preserves full-height wall acoustics, and sets fresh/staged-reset Dust Low on Android and desktop without overwriting saved Off/custom or genuine legacy preferences. Mobile resolution remains **50%**; 33%/40% stay experimental. Seven affected host suites, 256 Java tests/39 classes, lint, four-ABI/Windows builds and package checks pass. Exact SM-S948B/Android 16 install/pullback and normal entry pass with settings/menu mix unchanged; ordinary physical stand collision feedback remains pending. Owner waterfall, audio/haptics and 33% manual play acceptance are recorded; the long sustained phone programme is owner-deferred. Current ac97 source CI is pending, separate from earlier green runs. [Exact source/packages, regressions and limits](ENGINEERING_1_6_2_STAND_DUST_2026_10_09.md).

**Owner-accepted torch-water correction — 9 October:** exact `8edca4bb` bounds fire diffuse by the existing entrained-air fraction at the water material, preserving shared opaque lighting, warm highlights and Fresnel transport. 255 Java tests/39 classes, lint, four-ABI/Windows builds, three affected host checks and all 16 backend shader variants pass without widening frozen budgets. Sixteen matched phone pairs and eight Windows pairs cover near/far/oblique/catchment views on both hardware RT backends; saved settings/mix and captured resource payloads are unchanged. The owner accepts the waterfall appearance. Retained setup failures and owning-frame GPU observations are scoped explicitly; no sustained-FPS or causal performance claim. Refined runtime CI is pending publication/current-run results, separately from the isolated zero-diffuse source's 12/12 pass. [Exact source/packages, images and limits](ENGINEERING_1_6_2_TORCH_WATER_2026_10_09.md).

**Current owner decisions — 9 October:** ordinary dungeon play at experimental 33% is owner-approved as surprisingly good; 50% remains the mobile resolution default. The owner now requests Dust **Low by default in the final packaged build**, preserving saved Off/custom preferences; shared fresh/reset defaults and the stand collision fix pass integrated host/Java/build checks in `ac97da91`; exact normal-phone install passes, with physical stand feedback pending. This supersedes the earlier prototype-Off requirement. Owner audio and haptics are approved. The long sustained phone performance/thermal programme is deferred from this goal at the owner's direction; retain exact measured gaps and do not claim sustained 30 FPS. The torch-water correction is owner-approved; the reported player/Keeper stand clipping fix passes integrated host checks and is installed for ordinary phone feedback. Final integrated checks and Eric's independent audit remain required. No merge or release is authorised.

This ledger retains the historical 5 October pause checkpoint. The owner has
authorized post-reset standalone tomb work in [the active plan](ENGINEERING_1_6_2_TOMB_FINISH.md);
that later scope supersedes conflicting pause or future-version restrictions.
Merge and publication remain on hold. The [exact checkpoint](ENGINEERING_1_6_2_PAUSE_2026_10_05.md)
keeps its completed checks, glass-apply timing and package identities.

The owner's final 1.6.2 direction selects an explicit mobile default and a
bounded Mist On/Off option. These changes require new artifacts and affected
validation; earlier Keeper packages retain their recorded identities.

| Setting | New Android install / staged Reset Defaults | Windows default |
| --- | --- | --- |
| Resolution | 50% | 100% |
| Water | Mobile | High |
| Fire | Mobile | High |
| Preview cap | 30 Hz | 30 Hz |
| Physical glass | Off | On |
| Shadows | Current | Current |
| Mist | On | On |

Current indirect transport, Linear output filtering and the compiled Mobile/High
optical profiles remain unchanged. The mirror is retained on mobile and desktop;
the [matched mirror comparison](ENGINEERING_1_6_2_MIRROR_COST.md) establishes no
useful, repeatable saving from the isolated stone substitution. Desktop and High
mirror retention are explicit owner requirements.

The owner considers the existing phone appearance at 50% acceptable. Selecting
50% is a deliberate default choice, rather than an undisclosed response to a
slow frame. It does not establish sustained 30 FPS. Glass Off removes supported
physical glass independently of the immutable optical profile; Mobile already
omits the lantern panes. No measured saving is assigned to this final tuple.
Fire Low remains selectable, with its smaller integration budget, but lacks the
matched appearance/cost acceptance needed to make it the default.

## Storage and application

Platform defaults are distinct from the historical Accepted 1.6.1 baseline:
Android 75%, Mobile water/fire, cap 30, Glass On and Current shadows. That baseline
remains available and truthful. Valid saved custom, legacy and interrupted
pending settings retain their values; introducing a default does not rewrite
them. A new Android graphics configuration is detected from graphics-key
absence, even if audio, controls or progression already exist.

Schema4 adds Mist On/Off. Missing mist in schema1/2/3 saves and pending requests
migrates to On. Reset only stages a draft; the existing Use these settings,
Keep and save, timeout and Restore flow remains authoritative. The owner's
existing phone data is preserved during validation.

## Mist rendering contract

Mist On retains the corrected ordinary transport. Mist Off returns a neutral
ground-mist contribution before incident-source visibility checks and volume
integration. Flames, smoke, staff electricity, geometry, physical lighting and
the Keeper reveal/death/chest sequence retain their own authority. Off removes
the scene ground mist, rather than merely making its final colour transparent.

The existing sixteen-byte quality-control buffer uses a single disabled bit;
On retains its original zero upload value. Actual recorded, submitted and
completed mist evidence is separate from shadow quality and its reserved value.
Missing historical evidence remains unavailable. No new resource binding,
software rendering path, low-mist tier or measured performance claim follows.

SDK control-flow inspection confirms the compiled Off path bypasses ground-mist
source queries and marching. The required branch adds160 bytes/40 words/10
instructions to Generic and144 bytes/36 words/9 instructions to Opaque versus
their prior Pipeline artifacts, with two branches and one selection merge.
Loops, functions, function calls, ray-query sites and diagnostic atomics do not
increase. Five static ceilings per eight variants are updated to these exact
measured values; all other ceilings and categorical guards remain unchanged.
Budget SHA256 `f543692d968921d7e09df94e226da6579eeaebc44c92f8b79299a9624b04c220`
supersedes the historical `0f130f8f1e50623772bf857561adafee23e730e2691c35e31cb6455df4b258ba`
for this feature. This static-size change is not a dynamic timing result.

## Validation boundary

Implementation is complete at runtime source `1334cc9c58ec97940ac10d861f0397143ca7a9f4`.
The [runtime CI run](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37271761916)
passes all six individual jobs. Later documentation backups do not change these
artifact identities. Independent final review and the remaining owner/device
acceptance gates precede any release action.

Both Windows configurations and all four Android ABIs build. Twelve affected
Debug CTests pass in 51.61s, and three renderer/player integration checks pass
in 10.97s. Both affected Release shader fixtures pass in 123.55s. The complete
Android Java suite passes 154 tests in 24 classes with no failures, errors or
skips; lint reports 0 errors and 49 warnings. Pipeline8, Compute8, compatibility2,
ABI/adapter freshness and the56 Windows graphics source checks pass.

The pre-build seal covers 5,823 tracked files and 3,652,319,441 bytes. All sealed
source bytes remain identical after the builds. Each Windows ZIP closes 85
members; all three APKs pass four-ABI ELF/16KiB/compiler-policy/notices admission.
Actual selected PE and ARM64 shader modules pass SDK validation/disassembly;
Shipping has zero diagnostic atomics and no binding22. Unsigned Release fails
certificate verification as expected. No production signing was invoked.

Both Windows RT backends complete real Keeper-room On/Off/restored-On banks,
13 poses per bank, plus 14 preview images and 10 transactions each: 78 Showcase
and 28 preview readbacks. All eight launches exit 0 with synchronization validation
and no error markers. Canonical Showcase records join actual uploaded mist,
completed/present serials, selected payloads, 24 physical TLAS instances and 19
BLAS. Camera, geometry, gameplay phases, fire-light records and remaining quality
controls match across the modes. All 26 On and 26 restored-On RGBA images exactly
match the corresponding prior 646 Keeper images, and all 26 restorations match
new On. Off changes ground-mist contributions in seven checkpoints per backend.
The owner reviewed the original PNG comparison and found the optional Off still
appearance acceptable, with little visible difference in those views.

The compact preview's Mist Apply/Revert transactions pass current owning-frame
and desired/effective checks, with actual raw/completed mist checked by its
frozen native success gate. Its JSON does not separately serialize that optional
actual boolean. All three paused compact Mist images are identical because
that Skeleton alcove has no Keeper ground medium. These images cannot prove
Keeper-mist appearance or cost. Windows capture receipt
`c64c0156c230a40376a6be8eb121cceececef8276f7daafac49466442c93a777`.

On the allocated SM-S948B/Android16, the isolated Shipping validation app stages
the mobile reset tuple while its saved 75%/Glass-On tuple remains unchanged.
A real current RT frame acknowledges 50%, 720x1490 internal/1440x2980 output,
Mobile water/fire, Glass Off, Current shadows, Mist On; explicit Restore returns the
original tuple. Mist Off also has an actual requested/effective acknowledgement,
restores on Home/resume, and survives Use/Keep and a cold restart. Use/Keep On
and a second cold restart restore the original confirmed values. Only the
isolated validation app's graphics storage changes; no app data is cleared.
Fresh-install key-absence behavior is covered by CPU/Java migration tests;
the owner's existing app was not cleared to simulate a fresh install.

The exact new Debug APK on the same phone passes both Pipeline and
RayQueryCompute at the owner's saved 50%: 13 route waypoints, seven captures and
strict same-process Home/resume each. Actual installed pullbacks match
`cea6f9594696a7d97df85c2d6cbe7782b82f5a9c561f5a43bc93aa194bf1a11e`.
All 14 captures and both post-resume records carry actual completed Mist On,
Mobile fire 4/1/4, Current shadows 1/1, 24 TLAS instances and 19 BLAS. Surface
17->19 and scene epoch 18->20 join completed serial 2259 on Pipeline and 2242 on
Compute. Original Debug preferences remain byte-identical 1,420 bytes, preserving
saved 50%/Mobile-water/Mobile-fire/GlassOn/Current/cap 30, with missing old-schema
mist interpreted as On. Phone receipt
`76211a9987b5235fb4cf323eaa43f46e5c9c9209d4f6e7a8845f4fd947ed2237`;
settings/persistence receipt
`ff03e377b5185ecb0329df652c34c0357df344e70a0a9eb5d151b20539413a2d`.
Both validation apps were stopped after collection. Raw reports, images,
preferences and private identifiers remain outside Git.

Initial owned-UI traversal/stale-target failures and an insufficient first-ACK
poll remain private negatives. The corrected observer waits at most 120s for
scene-rebuild acknowledgement, preserving the ordinary 15-second post-ACK
confirmation deadline. A touch that did not stage Off was detected from the
actual requested value and is not counted as an Off pass. The private wrapper's
PowerShell argument-name and case-sensitive menu-label errors are retained;
continuations resume from verified actual state. No runtime guard was weakened.

These results certify affected correctness, persistence, package containment and
lifecycle on the named builds/devices. They do not prove sustained 30 FPS, a cost
saving from the new tuple or Mist Off, Windows Release execution, four
simultaneous real fire lights, early reward overlap, other phones, or new audio
acceptance. The significant measured Keeper cost and deferred optional choices
retain their recorded scope. Current 50% defaults are a deliberate owner choice.


Audio/haptic manual revalidation required: **NO** for these settings/rendering
changes, provided semantic events, listener/source data and playback remain
unchanged. Earlier changed-audio acceptance retains its own recorded scope.
