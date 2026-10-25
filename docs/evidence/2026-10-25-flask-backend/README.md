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
