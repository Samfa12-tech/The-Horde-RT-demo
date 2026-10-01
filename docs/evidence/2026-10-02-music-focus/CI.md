# Music focus and Android lane: current source/integration CI

Validated head `bdac9655efc2a6ec0abea4141ad11bab129de04e` includes focus
implementation `ef812d0`, Android CI `3cf25b3` and the SDK-path correction.
Fresh runs, not old-job reruns:

| Lane | [Push36892699876](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36892699876) | [PR36892707813](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36892707813) |
| --- | --- | --- |
| GCC portable |55/55 PASS |55/55 PASS |
| Clang portable |55/55 PASS |55/55 PASS |
| MSVC portable/native UI |57/57 PASS |57/57 PASS |
| Focused Vulkan CPU-host |15/15 PASS |15/15 PASS |
| Android Debug |38/38 PASS, four ABIs, lint/package PASS |38/38 PASS, four ABIs, lint/package PASS |

All ten jobs conclude success. Actual CTest/Java result lines and Android native
build tasks, lint and package checks were inspected. Java reports have zero
failures/errors/skips and include eight focus contracts. Android build steps take
3m58s push / 3m27s PR; these are build times, not game performance. Hosted Android
does not have a physical RT device or prove perceptual audio acceptance. Debug
uses the AGP debug key; no production signing credentials or publication.

PR15 synthetic merge `fb4cd17ff2cabd84098f683b5e7f26e7a1ac8749` has exact parents
`bda1b99a62e1de883273dd21f267bcfc92bc5490` and the validated head. Tree
`7ec51ac2675cc121588c0b893bf73c956f624f63` equals the head tree. PR is draft,
CLEAN/MERGEABLE at inspection; main is unchanged and has not been merged.

Keep the first687dda9 Android failures: push36892419452 / PR36892429964 exit127
before compilation because `sdkmanager` is absent from runner PATH. bdac965
resolves the already-installed tool under ANDROID_HOME and requires it executable;
no SDK upgrade, retrying an old job or weakening a check. Raw logs remain local:

| Receipt under `C:/Dev/tmp/horde-music-focus-20261002/` | SHA256 |
| --- | --- |
| ci-bdac965-push.log |a208f21352f38ef16af9f9be80bd2f7459808307b1a73c20d66a9c6ab1a8cf57 |
| ci-bdac965-pr.log |73d87cbd1d350371a98b085733d6e077fa8e453daba2f2847832ba40ed32a3f2 |
| ci-687dda9-android-push.log |9b556826bab1c77dacd874d14c50b1ca17935f52c6be2cb6a0ed7bc838a58516 |
| ci-687dda9-android-pr.log |01d1bf50b2371a2b41258c4a906f04ec55888ddcf0f20e8bdef9fa827576d596 |

The prior6ec30f0 MSVC report-UI timeout remains an unexplained intermittent
negative result; current passes do not prove its cause fixed. No assertions or
timeouts were weakened. Documentation-only receipt changes do not rebuild or
replace the admitted phone artifacts; consult PR15 for subsequent branch checks.

Audio/haptic manual revalidation required:YES for pending external-audio
interruption/return on the unchanged installed focus candidate, not for this
documentation or a new timbre audition. No repeated long-loop admission.
