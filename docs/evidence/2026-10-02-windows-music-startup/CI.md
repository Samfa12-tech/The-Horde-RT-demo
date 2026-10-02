# Current focus-repair CI receipt

Implementation source `5e20f109b20ff9613a7ed5bdce323d13cae2ddb3`.
Both fresh workflows completed SUCCESS; no old-job rerun:

- [Push37001774114](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37001774114)
- [PR37001780006](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37001780006)

Actual logs for all ten jobs are retained under ignored `reports/` and inspected.
PR checkout is `2c170c75a9e09fae3a0172a76b1d7fbe0dba710f`, whose verified parents
are main `bda1b99a62e1de883273dd21f267bcfc92bc5490` and the implementation source.
This validates the reconciled integration tree, without merging into main.

| Lane | Push | PR integration |
| --- | --- | --- |
| GCC portable CPU-host |59/59 PASS |59/59 PASS |
| Clang portable CPU-host |59/59 PASS |59/59 PASS |
| MSVC selected CPU-host |65/65 PASS |65/65 PASS |
| Focused Vulkan CPU-host player/resource |15/15 PASS |15/15 PASS |
| Android Debug |All4 native ABIs, Java76/76 with0 failures/errors/skips, lint and exact held-item/player APK admission PASS |Same checks PASS |

The new `horde_rt_windows_music_focus_tests` executes and passes in all six
compiler jobs. These are compilation/CPU-host/package gates, not physical RT,
phone performance or audible acceptance. Exact laptop Shipping243acc6b live RT,
native first-output and owner startup/menu/refocus acceptance are separate in
[the finite record](README.md). Android playback inputs did not change.

Later documentation-only closeout does not change the tested runtime, tests,
assets or CMake inputs. Preserve these exact source/run identities rather than
relabelling this receipt as validation of future implementation changes.
