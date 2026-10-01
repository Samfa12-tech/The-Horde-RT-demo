# Fresh report-integration checkpoint CI

## Windows helper / SDK checkpoint `52d3ed2`

Both branch heads are `52d3ed2001021e01d5017e6b845059c57275ad2c` at this
receipt. These are fresh source and integration runs, not old-job reruns.

- [Push36922430459](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36922430459): SUCCESS.
- [PR36922432778](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36922432778): SUCCESS.
- All ten actual job logs: each GCC56, Clang56, MSVC61, Vulkan CPU-host15 and
  Android75, zero failures/errors/skips. The Android aggregate is explicitly
  printed from the test XML. Four native ABIs, lint, runtime/licence package
  checks and required report-suite presence pass. Windows explicitly restores
  and verifies the pinned WebView2 SDK before configure; no runtime installation.
- PR15 draft/open/CLEAN. Synthetic integration
  `8036e0a426353b20426eb2de536d22ccc2e65b72` has exact main
  `bda1b99a62e1de883273dd21f267bcfc92bc5490` and checkpoint parents. Tree
  `3e418c865c9d34cc999c4dd9ce00525036db6d03` matches the checkpoint head.
- Actual raw push log `C:/Dev/tmp/horde-report-submission-20261002/ci-52d3ed2-push.log`
  SHA256 `68aff9f173258a9704008f0da183842ebd652ea9501eaf2d031111a9589d3579`;
  PR log `ci-52d3ed2-pr.log` SHA256
  `7bb9995a4aa97d1a96240d90638843590a55f065dc9a6fcb84330df2097388bf`.

This proves helper/SDK contracts and compilation, not later remote-form/app
integration, real challenge completion, phone interaction or email delivery.

## Android remote-form checkpoint `301105f`

Reviewed current head `301105f595bcb33ffc87968a90c0f2673247da74`, not an old-job
rerun. Both engineering/profile remote branch heads were verified identical.

- [Push36917677635](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36917677635): SUCCESS.
- [PR36917683622](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36917683622): SUCCESS.
- All ten actual job result logs inspected: each GCC56, Clang56, MSVC58,
  focused Vulkan CPU-host15 and Android75; zero failures/errors/skips. Android
  four-ABI build, lint and exact runtime/licence package guards pass. Required
  eight-suite Android receipt prevents silent omission of new report contracts.
- PR15 remains draft, CLEAN/MERGEABLE, not merged. Synthetic integration
  `429c7e1ce1153a18adc72bf3c7eaa549a545bf02` has exact main
  `bda1b99a62e1de883273dd21f267bcfc92bc5490` / head parents; tree
  `8f0b1e00d79b15c8d6266e00dd4e74d1fe70c838` is identical to the head.
- Actual raw receipts: `C:/Dev/tmp/horde-report-submission-20261002/ci-301105f-push.log`
  SHA256 `7a37792e996ec96a300a4611e8e6561281258572724e165081da5e13b9a8d274`;
  `ci-301105f-pr.log` SHA256
  `de48668573f817621ef82666f67623b9e6808418f59147dcfd86da7fab36b109`.

Compiler/host/CI evidence, not physical phone verification, real Turnstile,
email delivery or Windows remote transport/form acceptance. No main merge,
game release or publication. Later source changes require their affected gates;
these receipts are not implicitly current for a later Windows implementation.
