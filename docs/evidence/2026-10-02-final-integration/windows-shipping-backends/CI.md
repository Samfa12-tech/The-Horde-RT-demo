# Fresh checkpoint CI —172cbb9

Source172cbb93c8e7a03e66a09520c7c4aaedb03c6d9f is pushed and remote-verified
on both engineering/profile branches. PR15 remains draft/open/MERGEABLE;
`mergeStateStatus=CLEAN` after completion. No main merge occurred.

- [Current push37004882762](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37004882762):
  SUCCESS after one failed-jobs-only retry, attempt2.
- [Current PR37004890241](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37004890241):
  SUCCESS on the first attempt, all five jobs. Actual CI checkout19e5f19 matches
  fetched PR merge19e5f19a095f532c06a716e43205f26f61f3ab6f with verified parents
  mainbda1b99a62e1de883273dd21f267bcfc92bc5490 and source172cbb9 above.
  This is current reconciled-tree validation, not a historical-job rerun.

| Lane | Fresh PR evidence | Push disposition |
| --- | --- | --- |
| GCC portable host |59/59,77.24s |SUCCESS first attempt, reused not rerun |
| Clang portable host |59/59,81.67s |SUCCESS first attempt, reused not rerun |
| MSVC portable host |65/65,103.50s |First64/65 with UI timeout; one unchanged failed-job retry65/65,105.92s |
| Vulkan CPU-host player/resource |15/15,8.11s |SUCCESS first attempt, reused not rerun |
| Android Debug |Four native ABIs,Java76 total/0 failures/errors/skips, lint/build/exact package PASS;2m21s build |First fails SDK installation before compile; retry actual build3m10s,Java76/76 and exact package PASS |

Fresh job logs inspected from GitHub job-log endpoints. Generated full logs are
retained outside Git at `C:/Dev/tmp/horde-172cbb9-ci`; concise exact relevant
observations are [retained below](ci-observations.txt). CI is compile/host/asset
admission evidence, not GPU/device/presentation/performance/owner acceptance.
Actual Windows game builds and the four native Shipping rows are separate in
this record. Android physical rows remain OPEN.

## Negative rows kept, no unexplained source change

First push Android job110830701347 fails during SDK manifest download:
source lists/IO manifests unavailable, then missing `platforms;android-34`.
No Android compile/test was reached. PR's independent SDK/build lane succeeds.
No workflow or dependency-version change is justified by that download failure.

First push MSVC job110830701481 reports ERROR_TIMEOUT1460 on first note
`WM_SETTEXT` (control106/message0xc), phase note-only default/private-content
checks. Its64 passing tests do not make this UI row pass. PR's same-source
merged-tree UI row passes0.65s; retry UI row passes0.62s with the original
message/test bounds. Bounded read-only fixture/handler review found no proved
mechanism for the isolated timeout. The earlier startup-readiness fix is already
present. No timeout increase, retry-in-test, diagnostic suppression or UI fix
is introduced; these subsequent passes show intermittency, not its root-cause
repair. Future recurrence needs its own demonstrated discriminator.

An initial attempt to retry the failed Android job while its workflow was still
running is rejected (`job ... cannot be rerun`), with no job launched. After
completion, one `gh run rerun 37004882762 --failed` launches only Android and
MSVC; their job IDs110833213563/110833213746 pass. Other three lanes are reused.
No completed physical workload, old-source job or full Host audit was repeated.
Audio/haptic manual revalidation:NO; no new runtime behavior changed here.
