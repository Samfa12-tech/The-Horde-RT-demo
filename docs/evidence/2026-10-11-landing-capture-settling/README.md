# Landing capture settling

Follow-up to Kit admission on `397ace03`. This corrects diagnostic staging only;
it does not change live gameplay, held-item collision, quality or shader budgets.

The landing sweep froze immediately after ascent and a view-direction change.
Unlike later route stations, it omitted the existing two-second fixed-step
settling period. The physical lantern retained its old world orientation, so
east/south captures failed the actual final GripRing contact and visibility
assertions. The same 120 ordinary fixed ticks now run for landing views too.
No contact/visibility assertion is skipped or relaxed.

Actual hidden Windows RayTracingPipeline captures, RTX 5050 Laptop GPU,
960x540 at 100%, High bundle; identical checkpoints before/after. All eight
images were opened and inspected.

| View | Before | After | Outcome |
|---|---|---|---|
| East | [image](east-before.png), [record](east-before.json) | [image](east-after.png), [record](east-after.json) | Grip/light ownership and primary visibility pass after settling |
| South | [image](south-before.png), [record](south-before.json) | [image](south-after.png), [record](south-after.json) | Grip/light ownership and primary visibility pass after settling |
| North, toward shaft | [image](north-before.png), [record](north-before.json) | [image](north-after.png), [record](north-after.json) | Still fails: zero primary-arm pixels |
| Down shaft | [image](shaft-before.png), [record](shaft-before.json) | [image](shaft-after.png), [record](shaft-after.json) | Still fails: zero primary-arm pixels |

The remaining failures are not attributed to settling. The current exterior
carry-clearance helper uses ground/movement eligibility, which rejects the
open shaft as an obstruction. Separating empty space from actual solids needs
its own behavioral and visual checks; that correction is not implemented here.
The north frames also plainly retain the old overhead rope frame. Graveyard,
ground stake, rim-contact traversal and complete chapter acceptance remain open.

Native authored-terrain journey tests pass in rebuilt Debug and Release
([Debug](landing-settle-debug-tests.log), [Release](landing-settle-release-tests.log)).
Both application configurations build. [Exact identities](identities.json)
record the source, executables and unchanged shader budget. The previous Kit
APK remains the Android candidate; no installation or physical acceptance is
claimed. QueryCompute landing comparisons remain pending; Kit reunion/departure
was separately inspected on both backends.
