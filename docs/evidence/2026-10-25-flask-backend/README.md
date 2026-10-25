# Flask recovery backend — integration prerequisite

Implements the pure recovery state from the approved
[flask policy](../../design/HORDE_1_7_FLASK_POLICY_PROPOSAL_2026_10_24.md).
This checkpoint does **not** enable drinking, Rest, the upgrade, inputs, props,
HUD or campaign persistence in normal gameplay.

New games have two charges. A stationary eligible drink completes after 1.25
gameplay seconds, consumes one charge and returns one heart of bounded healing.
An eligible two-second Rest fully heals and refills current capacity. Cancellation,
accepted damage, death or eligibility loss wins a completion tie without charge
use. Repeated starts cannot restart an action. The one-time upgrade increases
capacity to three without granting health or charges. Restore validates resources
and drops transient progress; invalid restore leaves state unchanged.

`PlayerVitals::TryHealHalfUnits` preserves exact half hearts, the three-heart cap,
damage immunity/feedback clocks and life phase. It cannot resurrect. The future
simulation integration must apply completion healing and resource changes in the
same owner tick before publishing; this helper is not a second gameplay owner.

Windows MSVC Debug build and the two affected CTests passed:
`horde_rt_healing_flask_tests` (2.55 s) and
`horde_rt_showcase_gameplay_smoke` (3.44 s), unchanged 30-second gate for the new
test. Raw local log: `reports/held-guard-20261025/flask-backend-tests.log`.
No save migration, device or visual acceptance is claimed.

Next: integrate simulation-owned eligibility/cancellation and exact resources,
then versioned slot persistence/migration and the approved contextual Drink/Rest
input. Preserve rope interaction priority and left-hand light ownership. Verify
old-save recovery at each earned safe anchor before admitting migration.

## Dormant schema3 prerequisite

The codec now supports an exact 148-byte payload / 168-byte file containing
fractional health, upgrade entitlement and charges. Schemas1/2 retain their literal
golden bytes and decode missing resources as empty, unupgraded capacity2; no
write-on-read or automatic fill is added. Malformed booleans, excessive charges,
schema/length mismatch, truncation and unknown newer schemas reject without
mutating the caller's value or encoded output.

**Runtime admission remains blocked:** Begin/Commit restore explicitly reject
schema3 until the simulation owns and restores flask resources. Normal capture
still emits schema1/2. A focused test proves rejection preserves a pre-existing
valid pending restore and the live session, preventing silent resource loss.
This is not an implemented migration or player-facing flask feature.

The seven affected native CTests passed, including codec and actual-terrain
campaign restore. The campaign fixture took30.50s under its existing deadline;
no timeout or storage publication behavior changed. Local storage execution
remains blocked by the previously retained FileRenameInfoEx access denial and
was not retried. The next step is simulation-owned actions/resources, then
removal of this temporary schema3 guard only when exact restoration is covered.
