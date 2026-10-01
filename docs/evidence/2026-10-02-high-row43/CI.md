# Normal engineering CI, not candidate admission

Normal source `ccb61cc8723a6d674d5266297df2874e9643f332`, unchanged runtime from
accepted `a32a718`. Fresh [push36900311413](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36900311413)
and [PR36900320256](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36900320256)
both SUCCESS; all ten actual job logs inspected, not an old-job rerun.
Each: GCC55/55, Clang55/55, MSVC57/57, Vulkan CPU-host15/15 and Android38/38
(zero failures/errors/skips), all four Debug native ABIs, lint/build and actual
held-item/player APK payload/licence checks PASS.

Synthetic merge `518a77c65e2266f7d9057de1949c033ef517a538` has exact parents
`bda1b99a62e1de883273dd21f267bcfc92bc5490` and the above head. Tree
`383bded8811d2a88cb970fd4aa0f3664a1440cb2` equals the verified normal head tree.
PR15 is draft, CLEAN/MERGEABLE, not merged or published.

Raw actual logs at `C:/Dev/tmp/horde-high-row43-current-20261002`:

- `ci-ccb61cc-push.log` SHA-256 `66058632fec667062f8f64919b630dfdc37db76d0b7e07d054589ae2a7efff9f`.
- `ci-ccb61cc-pr.log` SHA-256 `1758aba7938c48927d9f00ed8805360fea26e8b1a895170c054e01b767cc28db`.

These checks do not admit the separately staged precision candidate, prove
physical RT/device acceptance, fix the earlier native report timeout's unknown
cause, close High/backend gates, or certify S24/S25 from another device.
Further receipt-only commits retain their own workflow status; do not use this
record as a claim that a different SHA has passed its jobs.
