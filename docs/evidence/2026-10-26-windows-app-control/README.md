# Windows execution block: read-only diagnosis, attribution pending

The exact blocked file is unchanged: `candidate-grouped/HordeLanternRT.exe`
under `reports/forest-branch-crown-r4-20261026`, SHA256
`95e95e681a21e4a2da352a30cd6c6ddbaefeca875a23cf2accdf972de8fa8eac`,
14,986,752bytes. Authenticode reports **NotSigned**. Its only NTFS stream is
`:$DATA`; no `Zone.Identifier` stream was present. These are facts about the
file, not identification of the enforcing policy. The generic script-oriented
signature message is not evidence that PowerShell execution policy caused the
Python `CreateProcess` failure.

## Exact additional diagnostic boundaries

The current tool token is non-administrator. A narrow Code Integrity Operational
query covering08:04:52–08:14:52UTC around the08:09:52UTC failed-launch log was
**denied**: `Attempted to perform an unauthorized operation.` It did not read
event contents; the empty result must not be described as no matching events.
No alternate log reader, event-file access or elevated retry was attempted.

The independent normal-privilege inventory query, Microsoft-signed System32
`CiTool.exe -lp -json`, was also **denied**, returning
`OperationResult:-2147024891` (`0x80070005`). No policies were read. No retry,
registry substitute, policy-file search or security change followed. See
[observations.json](observations.json). Smart App Control, an organization policy
and an execution-environment restriction remain unconfirmed; error4551 and an
unsigned binary alone do not identify which one enforced the block.

## Narrow proposed next action — approval required, not executed

Ask Sam or the authorized Windows administrator to approve a **read-only
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

## Remedy selection depends on that evidence

- If the actual enforced policy is Smart App Control, Microsoft's current
  [FAQ](https://support.microsoft.com/en-us/windows/security/threat-malware-protection/smart-app-control-frequently-asked-questions)
  offers no per-app exception. Its [developer guidance](https://learn.microsoft.com/en-us/windows/apps/develop/smart-app-control/code-signing-for-smart-app-control)
  supports signing with a trusted provider certificate. Any such owner-managed
  signing/account/spend decision needs separate authorization; no certificate,
  private key, trust store or artifact was changed. Self-signing and adding a
  local trusted root is not an equivalent approved remedy.
- If a managed App Control policy is responsible, its policy administrator must
  review the exact blocked file and permitted deployment mechanism. A narrowly
  scoped hash decision would trust that artifact; publisher/folder decisions
  trust a broader set. No allow rule or policy edit is proposed for execution
  until the actual policy and owner are known and the exact change is approved.
- If collection instead establishes an execution-environment restriction,
  request its normal permission process; do not alter Windows protections to
  compensate. Without attribution, no security change is justified.

Current official pages differ in historical mode-toggle descriptions; this
diagnosis does not rely on reversibility or propose disabling protection.
No old/copy/renamed/rebuilt executable was used to evade the block. Forest
candidates remain unadmitted, phone access absent, and game data untouched.
