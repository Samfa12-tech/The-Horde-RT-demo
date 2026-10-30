# Owner/admin read-only collection - completed once with approval

Sam approved this scope and the original Windows UAC prompt. The single approved
collection completed at09:18:20UTC on2026-10-26; see the matching sanitized result
and current diagnosis in [README.md](README.md). These retained commands are the
reviewed scope, not an instruction to repeat collection. Any new collection
needs its own justification/authorization. Use an administrator's normal
approved Windows PowerShell or Event Viewer session.
Do not change execution policy or use a bypass switch if a tool is refused.
These commands neither launch Horde nor change protection. They emit diagnostic
text only; review it privately before sharing relevant fields.

The event query is limited to ten minutes around the retained failed launch.
It selects the exact candidate suffix/hash and related signature events by
Correlation ActivityID. If512 events are returned, report the bound; absence
cannot be inferred from a truncated or denied query.

```powershell
$start = [DateTimeOffset]::Parse('2026-10-26T08:04:52.1872317Z').LocalDateTime
$end = [DateTimeOffset]::Parse('2026-10-26T08:14:52.1872317Z').LocalDateTime
$events = @(Get-WinEvent -FilterHashtable @{
    LogName = 'Microsoft-Windows-CodeIntegrity/Operational'
    StartTime = $start
    EndTime = $end
} -MaxEvents 512 -ErrorAction Stop)
$needle = 'forest-branch-crown-r4-20261026\\candidate-grouped\\HordeLanternRT\.exe|95e95e681a21e4a2da352a30cd6c6ddbaefeca875a23cf2accdf972de8fa8eac'
$matches = @($events | Where-Object { $_.ToXml() -match $needle })
$activityIds = @($matches | ForEach-Object {
    ([xml]$_.ToXml()).Event.System.Correlation.ActivityID
} | Where-Object { $_ })
$related = @($events | Where-Object {
    $xml = [xml]$_.ToXml()
    ($_.ToXml() -match $needle) -or
        ($xml.Event.System.Correlation.ActivityID -and
         $activityIds -contains $xml.Event.System.Correlation.ActivityID)
})
[pscustomobject]@{Examined=$events.Count;AtBound=($events.Count -eq 512);Matched=$related.Count}
$related | Select-Object TimeCreated,Id,RecordId,Message | Format-List
& "$env:WINDIR\System32\CiTool.exe" -lp -json
```

Useful fields: event timestamp,3077/3033/3089 IDs, file/flat hash, PolicyName,
PolicyId, Status, requested/validated signing levels and correlated signature
count; policy inventory FriendlyName/PolicyID/IsEnforced/PlatformPolicy.
No need to export unrelated application history, device identifiers, tokens,
private keys or complete security logs. If either query is refused, retain that
error and stop; do not switch to another reader or change access rights.
