# Normal Keeper texture lifetime

The chapter remains incomplete. After a completed Kit adoption, the renderer
now retires the Keeper's private base-colour/emissive texture pair as well as
the previously retired vertex buffer. Both Windows RT backends measure a real
**33,554,432-byte (32 MiB) device-local decrease**. Static tomb/forest geometry,
the shared 18-layer atlas and shared third-character AS remain resident:
**static-region residency saving is still zero**. This is a bounded actor
resource lifetime, not complete zone streaming.

## Publication, restoration and uncertain work

The Keeper-only hit path reads bindings8/9. Kit uses StaticPbr and does not read
those private samplers. A submitted Kit BUILD and completed graphics-work proof
precede the switch to compatible, retained static array views. The two image
descriptors switch before either private image is destroyed. Full descriptor
rewrites use the same alias policy; resize/recreation cannot rebind missing views.
Both platforms call the shared lifetime path before recording scene descriptors.

Reset/grounded loss prepares the complete original pair before publication.
Windows restores the same 2048 RGBA8 sRGB KTX2 files; Android keeps mandatory
2048 ASTC6x6 sRGB with no fallback. Runtime uploads have owned commands/fences
and a bounded two-second wait. Failed or stale unsubmitted preparation cleans
only temporary owners. An uncertain submit/wait, including an exception after
submission, retains both candidate images, staging, command and fence until
the existing platform teardown proves device idle. Move transfers these owners;
inventory counts their actual allocations. No fourth actor, new layer, shader
change, mask change or quality reduction is introduced.

This safe runtime path does not certify the older startup-only
RunOneTimeCommands uploader's failure cleanup. Its unproved-completion cleanup
hazard remains a separate engine follow-up; the new runtime restore does not use it.

## Actual RT round trip and inspected images

Debug Diagnostic, RTX5050 Laptop GPU, 960x540 at100% scale, Higher shadows and
High optical/water/fire quality. Each backend completes4,961 authoritative ticks,
1,241 moving RT frames, one retirement and one reset restoration. The unchanged
150-second route and180-second process bounds remain in force. This route starts
after staged Keeper death/first ascent; it is not a fresh complete combat playtest.

All27 PNGs per backend are byte-identical to the preceding vertex-lifetime
candidate on that same backend, including reset opening. All54 current images
were opened through these eight sheets. Landing360, visible/walking Kit, fresh
raise, bends, lookout360, return, descent/lower sweep and second ascent are covered.
No cross-backend pixel identity or new owner visual acceptance is claimed.

| Backend | Landing/Kit/bends | Lookout | Return/descent | Repeat/reset |
| --- | --- | --- | --- | --- |
| Pipeline | [Sheet1](native/pipeline/contact-1.jpg) | [Sheet2](native/pipeline/contact-2.jpg) | [Sheet3](native/pipeline/contact-3.jpg) | [Sheet4](native/pipeline/contact-4.jpg) |
| QueryCompute | [Sheet1](native/query/contact-1.jpg) | [Sheet2](native/query/contact-2.jpg) | [Sheet3](native/query/contact-3.jpg) | [Sheet4](native/query/contact-4.jpg) |

Actual device-local bytes decrease from405,550,288 to371,995,856 and return to
405,550,288 on reset. Both private image allocations have device-local flag1
and no host-visible flag. The earlier1,323,264-byte host-visible vertex saving
remains. Both skeleton/corpse owners and20 BLAS /1 TLAS /28 instances remain.

Reset adds exactly33,554,432 bytes to measured DynamicUpload, totaling39,074,544
bytes including ordinary current-pose uploads. Single Debug reset WholeFrameCycle
observations are120.3719ms/Pipeline and112.9704ms/QueryCompute; the map/copy stage
is9.1415/3.1769ms. These include decode/wait in whole-cycle time, but do not isolate
the upload's GPU execution or establish sustained/Shipping performance. Preparation
and hitch optimization remain required. [Raw comparison/accounting](checks/keeper-texture-image-review-r01.json).

The [terrain-light attribution](../2026-10-13-terrain-light-attribution/README.md)
remains valid: shared lighting, lighter ground albedo and stronger upward moon
response explain the contrast. No production material/light tuning was adopted.

## Checks, exact candidate and remaining gates

Behavioral tests first failed retirement/restoration and then quarantine/move/
inventory. Final Debug and Release each pass five affected CTests: cutover,
resource inventory, initialise preflight, GPU-work lease and character-slot smoke.
Fault coverage includes missing callbacks/aliases/sampler, stale/unsubmitted BUILD,
failed proof, partial/throwing restoration, uncertain upload, move and teardown.
Original torch assertions/coverage and its180-second policy are unchanged; this
slice does not claim a newly run local full torch gate.

Android four-ABI Debug build/lint passes. Fresh Java:286 tests /45 suites,
zero failures/errors/skips. Actual APK and held-asset package checks and the
unchanged development certificate pass. Only four native-library entry payloads
change versusf9d01fc4f53c; the APK roster, Java and assets are byte-identical.
Fresh eight-key shader compilation matches the frozen catalog and budgets.
Failed shader invocation/preset and red evidence remain in checks.

[Source/artifact receipt](source-and-artifact-receipt.json) records the pre-commit
base plus exact raw build-input hashes and canonical Git blob identities.
Windows Debug SHA:e88e078d7a2b095a590051391733459d97701f797adf7a7646a63893a49e21bc.
Windows Release SHA:100c3484dc77c0a02325ad7502e16152cde7d04c8a1f9b84504920d0bd2426c5.
Prepared **uninstalled** Debug APK:
525e73bbd3c3c77db3e1bfd5251a15ee0d54e2d705fa0cd1555813bcf9ec5a53,
183,169,738 bytes, versionCode10, unchanged development certificate.
Exact-commit CI results are reported on PR27 after the normal push.

Normal contributor-aware static-region preparation/adoption/retirement, optimized
sustained throughput, fresh phone allocation/route/Current-Higher/Lower-migration
checks and owner audio/haptic/touch/visual acceptance remain open. Android allocation
savings are unmeasured. Ground material balance stays open; Easy Tree/canopy and
mist issue28 stay deferred. Audio/haptic manual revalidation for this resource
slice: **NO**; semantics are unchanged, while the chapter's cue acceptance is pending.

Next owner route: opening/guards → Keeper → claim → rope ascent → fresh raise/Kit
→ bends/lookout → return/descent → second ascent → reset. Note any pop/hitch,
retained corpses, equipment/light ownership and ground/tree contrast.
