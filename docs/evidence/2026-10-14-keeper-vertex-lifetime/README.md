# Normal Keeper vertex lifetime

The chapter remains incomplete. This slice retires one real Vulkan vertex
allocation after Kit takes the dedicated third character slot, and recreates it
before any return to the Keeper owner. **Measured Windows device-local savings
remain zero.** Static tomb/forest geometry, textures and shared AS allocations
remain resident; this is not complete zone streaming.

## Ownership and failure behavior

Kit's StaticPbr hit path reads the shared static vertex/index/material buffers,
rather than the Keeper's separate binding-7 vertex stream. A successfully
submitted Kit BUILD must precede retirement. The owner waits the sole graphics
fence and resets older recorded command references, rebinds binding7 to retained
static storage, then destroys only the Keeper vertex buffer. The current command
is still unbound. Both Pipeline and QueryCompute use this descriptor layout.

Restoration creates, maps and uploads a candidate buffer before publication.
Failed preparation, stale generation or failed completion proof preserves the
old actor/descriptor and destroys only an unsubmitted candidate. Restoration
invalidates the cached Keeper skin, so unchanged clip/time cannot suppress the
new upload. The later current-frame record rebuilds the same shared third BLAS.
Grounded loss, route reset, move and reconstruction are covered. Both waterfall
skeleton/corpse owners, third BLAS/backing/scratch, textures and limits stay intact.
No shader or production image change is introduced.

## Actual native route and inspected gallery

Final r02 captures use Debug Diagnostic on the RTX5050 Laptop GPU, 960x540 at
100% scale, High optics/water/fire and Higher shadows. Each backend completes
4,961 authoritative ticks /1,241 moving RT frames, then an ordinary route reset.
The unchanged 150-second route and180-second process bounds remain in force.

Each gallery has27 frames: landing360 sweep, fresh lantern raise, visible
walking Kit, bends, lookout360 sweep, return, descent/lower sweep, second ascent
and reset opening. All26 pre-reset PNGs are byte-identical to the preceding
ec43 route on their own backend. This does not claim cross-backend byte parity.
All54 final images were opened through the eight labelled contact sheets.
Complete PNG originals remain under ignored reports; selected originals and raw
camera/state/completed-frame evidence are attached here.

| Backend | Landing/Kit/bends | Lookout | Return/descent | Repeat ascent/reset |
| --- | --- | --- | --- | --- |
| Pipeline | [Sheet1](native/pipeline/contact-1.jpg) | [Sheet2](native/pipeline/contact-2.jpg) | [Sheet3](native/pipeline/contact-3.jpg) | [Sheet4](native/pipeline/contact-4.jpg) |
| QueryCompute | [Sheet1](native/query/contact-1.jpg) | [Sheet2](native/query/contact-2.jpg) | [Sheet3](native/query/contact-3.jpg) | [Sheet4](native/query/contact-4.jpg) |

Actual allocation decreases by1,323,264 host-visible bytes and one buffer/memory
allocation per backend. Device-local bytes remain405,550,288; BLAS/TLAS counts
remain20/1 and instance count28. Each route retires exactly once and restores
exactly once on reset. The restored1,323,072-byte payload is included in DynamicUpload;
allocation padding explains the difference. Single Debug reset observations are
in [the accounting record](restored-upload-accounting.json), not sustained or
Shipping performance acceptance.

The [terrain-light investigation](../2026-10-13-terrain-light-attribution/README.md)
still explains the bright ground: shared lighting, lighter ground albedo and
upward moon response. Its production lighting/material tuning remains unchanged.

## Checks and exact candidate

Failing red ownership tests are retained. Windows Debug/Release apps build;
eight affected CTests pass in each initial lane, with four Debug ownership/skin/
inventory/preflight rechecks after completing upload accounting. The original
torch gate keeps all legacy/roof cases and its180-second policy; Debug passes
in163.07seconds. Fault tests cover failed proof, exceptions, stale/unsubmitted
adoption, incomplete restore, retries, exact inventory, move/reset and fresh skin.
An independent read-only lifetime/control-flow review found no consequential defect.

Android four-ABI Debug build/lint passes. Fresh Java tests pass286/286 across45
suites, zero failures/errors/skips. Actual APK/held-item inventory and development
certificate pass. Only four native-library payloads differ from the preceding
1b508d6ae48d APK; Java/assets/entry roster match. Production shaders and frozen
budgets are unchanged from the preceding eight-key freshness check. Exact-commit
CI is reported on PR27 after the push.

[Artifact/source receipt](source-and-artifact-receipt.json) records pre-commit
builds against base8f048cf5 plus exact changed-source hashes; these are not claimed
as post-commit rebuilds. Current prepared **uninstalled** Debug APK:
`f9d01fc4f53ca75719fbfca9cc82d3afcfaee5e1a2992e9bd41b4c52671370a2`,
183,145,434bytes, versionCode10, unchanged development certificate.
Windows Debug SHA: `30af7fa4ec92e886d4f0d902d05200b441e30e391367023c0dbbc79e01e19544`;
Release SHA: `c24b7ed3888ac4e9e5337e10034efb9c67274b68487870b18bae716ecddbb886`.

Conservative static-region eligibility, actual normal two-way GPU loading/
adoption/retirement, sustained Shipping and fresh phone/owner acceptance remain
open. Phone Current/Higher and persisted Lower→Current acceptance remain pending.
No device action occurred. Audio/haptic manual revalidation for this resource
slice: **NO**; the chapter's changed-cue acceptance remains pending. Easy Tree/
canopy and mist issue28 remain deferred. No merge or release.

Next owner route: opening/guards → Keeper/claim → rope ascent → fresh lantern
raise/Kit → bends/lookout → return/descent → second ascent → menu reset. Report
any transition hitch/pop, missing corpse/Kit, equipment/light problem or stale
dialogue, plus ground/tree contrast at unchanged settings.
