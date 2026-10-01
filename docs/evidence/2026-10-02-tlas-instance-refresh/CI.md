# Current source and reconciled integration CI

Validated head `46afd732e3762bb1aa34865aef750168b0fabc85` includes renderer
`9f4042f` and exact phone/RTX receipts. Fresh runs, not old-job reruns:

| Lane | [Push36883566312](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36883566312) | [PR36883575417](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36883575417) |
| --- | --- | --- |
| GCC portable |55/55 PASS |55/55 PASS |
| Clang portable |55/55 PASS |55/55 PASS |
| MSVC portable/native UI |57/57 PASS |57/57 PASS |
| Focused Vulkan CPU-host |15/15 PASS |15/15 PASS |

All eight jobs conclude success; lead inspects actual CTest result lines,
including `horde_rt_scene_resource_inventory_tests` passing in both Vulkan lanes.
This contains discrete TLAS-definition/motion/submit-cache ownership cases;
CPU-host success does not certify hardware RT, device performance or owner feel.

PR15 merge `a4758ad12e5c51e68fa0a7e073b7f48420ecf498` has exact parents
`bda1b99a62e1de883273dd21f267bcfc92bc5490` and the validated head, tree
`dbcafd4bb7c834e0df21fb916d82bbb450417235` identical to head. PR is draft,
CLEAN/MERGEABLE at inspection; no main merge or publication.

Raw logs retained locally (public build logs, not device reports):

- `C:/Dev/tmp/horde-s24-instance-hits-20261001/ci-46afd73-push.log`, SHA256
  `6d2f5ea9269f6d5dd6504109770d66c8c4f5248a6b91dffd77ee508f323fefae`.
- `C:/Dev/tmp/horde-s24-instance-hits-20261001/ci-46afd73-pr.log`, SHA256
  `fe6a01cb5739b43aaa97a0f026cb23c902462e4ab42125fac4993558c67eec69`.

Keep the negative docs-only6ec30f0 push36879087706 result: MSVC56/57,
report-UI WM_SETTEXT to note control106 timed out (ERROR_TIMEOUT1460), while
the other lanes and same-source PR36879098824 passed. No concrete deadlock or
unique stall cause was found in bounded source review. Current passes do not
prove the intermittent fixture problem fixed; no assertion/timeout was weakened.
The earlier0cf4f05 green result is historical, not this renderer's admission.

Subsequent documentation/receipt-only commits do not rebuild or replace these
frozen device/executable artifacts. Consult PR15 for the latest branch checks.
Audio/haptic manual revalidation required:NO (renderer/test/doc slice only).
