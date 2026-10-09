# KIT ANIMATION PLAN 1 7 — WP0 custody and reconciliation

Reconciled 9 October 2026. The complete latest document is retained as an [immutable external planning reference](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md), verified against its Git blob in the [source manifest](superpowers/plans/2026-10-09-horde-1.7-wp0-source-manifest.json). This local summary imports no runtime or source assets. Read with the [reconciled master](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md) and [item dispositions](superpowers/plans/2026-10-09-horde-1.7-wp0-runtime-reconciliation.md).

Kit candidate selection and idle/walk/turn/gesture proof are separate from player forward/back walk/run/strafe/dodge/carry/climb needs. Catalogue IDs are hypotheses until entitlement/retarget/native-motion gates. Fixed-anchor rope deployment during pickup needs no throw or release marker. No acquisition or production is authorized.

References to queued 1.6.2 guards, equipment, openings, base UI/hearts and graphics in the original are historical: retain the accepted main implementation, not old missing-work claims. Rights/cost/recording/runtime/device gates remain open where stated. Exact trail/asset counts are planning proposals, not certified budgets.

## Section-by-section source custody

| Pinned section | Custody |
|---|---|
| <a id="simpler-rope-staging"></a>Simpler rope staging | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L11) |
| <a id="what-kit-needs-to-communicate"></a>What Kit needs to communicate | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L15) |
| <a id="candidate-model-and-first-validation"></a>Candidate model and first validation | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L27) |
| <a id="production-order"></a>Production order | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L33) |
| <a id="kit-animation-matrix"></a>Kit animation matrix | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L43) |
| <a id="conditional-work"></a>Conditional work | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L57) |
| <a id="rope-deployment-without-a-character-clip"></a>Rope deployment without a character clip | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L63) |
| <a id="acquisition-shortlist-and-rights"></a>Acquisition shortlist and rights | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L67) |
| <a id="useful-meshy-candidates"></a>Useful Meshy candidates | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L79) |
| <a id="meshy-and-blender-production-workflow"></a>Meshy and Blender production workflow | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L92) |
| <a id="meshy-intake-check"></a>Meshy intake check | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L100) |
| <a id="production-steps"></a>Production steps | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L106) |
| <a id="free-does-not-mean-every-tier-is-included"></a>Free does not mean every tier is included | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L118) |
| <a id="acceptance-checks-and-player-appendix"></a>Acceptance checks and player appendix | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L122) |
| <a id="kit-acceptance-checklist"></a>Kit acceptance checklist | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L124) |
| <a id="player-directional-movement"></a>Player directional movement | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L136) |
| <a id="sources-and-verification"></a>Sources and verification | [Immutable source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md#L165) |
