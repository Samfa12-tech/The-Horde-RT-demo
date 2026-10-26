# Windows execution block: Smart App Control confirmed

## Later owner launch and Computer Use observation

The enforced automated launch failure below remains valid for its recorded
time and context. It does **not** establish that this executable cannot run in
every context. Sam subsequently opened the exact `candidate-grouped` executable
manually through Explorer and reported that it was running. No protection
change was reported or performed by this task.

The supported Computer Use inventory independently returned one running Horde
window, titled `Horde Lantern RT - Showcase Alpha 1.6.2`, whose process-backed
application identifier names the exact original `candidate-grouped` executable
path. A read-only hash check still matched the original flat SHA256 below.
This is running-window/path evidence, not a screenshot, verified loaded-module
hash, RT presentation receipt, candidate visual acceptance or automated
recreation pass.

Selecting that existing window for read-only inspection timed out with
`Computer Use app approval timed out`. Sam reported repeated approvals and
explicitly authorized Computer Use of Horde. Checking the existing JavaScript
binding then showed that it had not been created. No duplicate app request,
input, screenshot, alternate process-inspection route or launch followed.
The owner-launched process was left untouched. The current visual-review
blocker is therefore the Computer Use approval/binding failure; the earlier
automated launch remains unretired and must not be retried without its own
authorization.

The earlier Markdown `/C:/Users/...` link is not a literal Windows Explorer
path. Sam's successful Explorer path resolution is separate from the recorded
`CreateProcess` failure and correlated Code Integrity event. Do not use the
link-format mistake to erase that event, or infer from the manual success that
Smart App Control switched off, a policy changed, or cloud reputation changed.
The original approved event query covered ten minutes around the failure and
contained no policy-activation event. It cannot date an Evaluation-to-On
transition or establish a Windows-update cause. No broader event query was run.

A read-only version query reported kernel `10.0.26300.0`, registry build/UBR
`26300.9711` and display version `26H2`; the registry ProductName still says
`Windows 10 Home`, so that label alone is not used to identify the installed
release. These values do not date a policy or update transition. Microsoft's
current FAQ describes re-enabling on some recent updates, but Sam's actual
Smart App Control page warned that Off cannot be reversed without reinstalling
Windows. Treat the machine-visible warning as binding here; reversibility on
this installation was not established and no toggle was tested.

The available evidence establishes SAC trust enforcement against an unsigned
binary, not an antivirus/malware verdict. Unknown unsigned code can be blocked
under Microsoft's documented rules. Neither universal install failure for all
players nor a guarantee of safety follows. Windows distribution/signing remains
an open release consideration; no external scanning upload, signing setup,
purchase, exemption, security change or public distribution occurred.

## Approved administrator result

Sam approved the narrow read-only administrator collection and then approved its
original Windows UAC prompt. That single request completed on2026-10-26 from
09:18:18.8843469 to09:18:20.0391863UTC. The launch session returned successfully;
no second elevation request was made. The collector did not launch Horde or
change protection. Its raw result stays in ignored private reports; only the
matching Horde fields and relevant policy identity are retained in the
[sanitized result](approved-administrator-result.json).

The ten-minute query examined4 events, below its512-event bound. Event3077 at
08:09:55.3741581UTC identifies the exact blocked candidate and its matching
SHA256 flat hash. It names **VerifiedAndReputableDesktop**, GUID
`{0283ac0f-fff1-49ae-ada1-8a933130cad6}`, status`0xc0e90002`, requested signing
level2 and validated level1. Correlated3089 records report zero signatures.
The separate `CiTool.exe -lp -json` inventory succeeded with OperationResult0
and reports that same system policy **IsEnforced=true**, **IsAuthorized=true**;
the evaluation policy is not enforced. Missing/null inventory fields are not
interpreted as false.

Microsoft's [inbox policy table](https://learn.microsoft.com/en-us/windows/security/application-security/application-control/app-control-for-business/operations/inbox-appcontrol-policies)
identifies that exact GUID/name as Smart App Control turned on. Its
[event reference](https://learn.microsoft.com/en-us/windows/security/application-security/application-control/app-control-for-business/operations/event-id-explanations)
identifies3077 as an enforced block and3089 as correlated signature information.
Together these establish the enforcing policy; the earlier error number alone
did not. They do not establish a malicious-file verdict, the cloud reputation
decision, or the absence of unrelated policies on the device.

## Supported remedy - separate owner decision required

**Owner disposition after diagnosis:** Sam has no existing Windows signing
certificate/service and asked to leave this issue aside. Stop pursuing signing
or setup for now. Keep the blocked native visual check pending and its candidate
unadmitted; continue independent permitted development. This is not permission
to ignore enforcement, change protection or retry via another executable path.

The recommended developer route is valid Authenticode signing with a certificate
from a trusted provider, keeping Smart App Control enabled. Microsoft's current
[FAQ](https://support.microsoft.com/en-us/windows/security/threat-malware-protection/smart-app-control-frequently-asked-questions)
does not provide a per-app exception; its
[signing guidance](https://learn.microsoft.com/en-us/windows/apps/develop/smart-app-control/code-signing-for-smart-app-control)
requires a trusted provider and recommends Artifact Signing. An exact-file hash
allow rule is therefore not the supported per-app remedy for this confirmed SAC
policy. Self-signing or installing a new trusted root is not an approved substitute.

**If the owner reopens this dependency:** choose a trusted Windows signing
identity, if one exists, or separately approve an eligible provider and cost.
The proposed eventual operation is to sign the exact reviewed development
artifact and required dependencies with that identity, retaining SAC and the
unsigned evidence. Identity availability, provider eligibility, cost and required
dependency signatures are not yet established, so no signing command is ready
for execution. Do not inspect/export private keys or recovery material, enroll an
account, purchase a certificate/service, sign an artifact, or launch the blocked
file without the corresponding explicit authorization. Any eventual signing
must preserve the unsigned evidence, record the resulting new artifact hash and
then pass signature/dependency and actual-launch validation; signing is not a
runtime acceptance claim. Disabling SAC affects protection for other apps too
and is not proposed or performed by this checkpoint.

The native visual trial remains blocked pending that supported remedy and its
separate approval. Forest/ring/flask candidates and their admission gates remain
unchanged. No security policy, permission, registry, trust store, signature,
game data or phone state was changed.

## Original file and ordinary-token attempts, retained history

The exact blocked file is unchanged: `candidate-grouped/HordeLanternRT.exe`
under `reports/forest-branch-crown-r4-20261026`, SHA256
`95e95e681a21e4a2da352a30cd6c6ddbaefeca875a23cf2accdf972de8fa8eac`,
14,986,752bytes. Authenticode reports **NotSigned**. Its only NTFS stream is
`:$DATA`; no `Zone.Identifier` stream was present. These are facts about the
file, not identification of the enforcing policy. The generic script-oriented
signature message is not evidence that PowerShell execution policy caused the
Python `CreateProcess` failure.

### Initial diagnostic boundaries

The current tool token is non-administrator. A narrow Code Integrity Operational
query covering08:04:52–08:14:52UTC around the08:09:52UTC failed-launch log was
**denied**: `Attempted to perform an unauthorized operation.` It did not read
event contents; the empty result must not be described as no matching events.
No alternate log reader or event-file access followed that initial denial. The
later approved administrator collection is recorded above.

The independent normal-privilege inventory query, Microsoft-signed System32
`CiTool.exe -lp -json`, was also **denied**, returning
`OperationResult:-2147024891` (`0x80070005`). That attempt read no policies. No
registry substitute, policy-file search or security change followed. See the
historical fields in [observations.json](observations.json). Attribution was
unconfirmed until the separately authorized collection above.

### Original diagnostic proposal, subsequently approved and completed

The proposal asked Sam or the authorized Windows administrator to approve a **read-only
administrator collection** of the two sources, through their normal managed
Windows tools. [The reviewable commands](owner-read-only-commands.md) select only
the relevant Horde event window and list current policy identities/enforcement.
This is a request for diagnostic visibility, not permission to launch Horde,
sign code, deploy rules, grant event-log rights or turn anything off. Policy
names/signers may reveal administrative information; keep the result private
and share only the Horde event/policy fields needed for diagnosis.

Microsoft's [debugging guide](https://learn.microsoft.com/en-us/windows/security/application-security/application-control/app-control-for-business/operations/appcontrol-debugging-and-troubleshooting)
uses elevated diagnostic collection and correlates block/signature events to
policy identity. Its [event reference](https://learn.microsoft.com/en-us/windows/security/application-security/application-control/app-control-for-business/operations/event-id-explanations)
identifies3077 enforcement,3089 signature details and3099 activation. The
[CiTool reference](https://learn.microsoft.com/en-us/windows/security/application-security/application-control/app-control-for-business/operations/citool-commands)
defines `-lp` as policy inventory; mutation switches are not part of this request.

### Original conditional remedy assessment

- For Smart App Control, now confirmed, Microsoft's current
  [FAQ](https://support.microsoft.com/en-us/windows/security/threat-malware-protection/smart-app-control-frequently-asked-questions)
  offers no per-app exception. Its [developer guidance](https://learn.microsoft.com/en-us/windows/apps/develop/smart-app-control/code-signing-for-smart-app-control)
  supports signing with a trusted provider certificate. Any such owner-managed
  signing/account/spend decision needs separate authorization; no certificate,
  private key, trust store or artifact was changed. Self-signing and adding a
  local trusted root is not an equivalent approved remedy.
- Had a different managed App Control policy been responsible, its administrator would
  review the exact blocked file and permitted deployment mechanism. A narrowly
  scoped hash decision would trust that artifact; publisher/folder decisions
  trust a broader set. No allow rule or policy edit is proposed for execution
  until the actual policy and owner are known and the exact change is approved.
- An execution-environment restriction was another original possibility. The
  matching SAC enforcement is now established; no attempt to compensate by
  changing unrelated protections is authorized.

Current official pages differ in historical mode-toggle descriptions; this
diagnosis does not rely on reversibility or propose disabling protection.
No old/copy/renamed/rebuilt executable was used to evade the block. Forest
candidates remain unadmitted, phone access absent, and game data untouched.
