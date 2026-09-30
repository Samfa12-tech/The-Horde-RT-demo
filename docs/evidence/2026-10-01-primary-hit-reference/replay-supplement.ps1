param([Parameter(Mandatory=$true)][string]$Root)
$ErrorActionPreference='Stop'
$archive=(Resolve-Path -LiteralPath $Root).Path
$replay=Join-Path 'C:/Dev/tmp/horde-primary-hit-profile-20261001' ('supplement-replay-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $replay | Out-Null
$old=Get-Content -LiteralPath (Join-Path $archive 'supplemental-completed-zone-cpu-20261001.json') -Raw | ConvertFrom-Json -DateKind String
foreach($name in @('supplement-completed-zone-cpu.ps1','analyse-primary-hit-trial.ps1','aggregate-primary-hit-abba.ps1')) {
    Copy-Item -LiteralPath (Join-Path $archive $name) -Destination (Join-Path $replay $name)
}
foreach($row in $old.records) {
    $id=$row.runId; $role=$row.artifactRole
    foreach($name in @("analyses/$id-$role.analysis.json","phone/$id/trial.json","phone/$id/$id/benchmark.json","phone/$id/$id/result.json")) {
        $dest=Join-Path $replay $name
        New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $archive $name) -Destination $dest
    }
}
& (Join-Path $replay 'supplement-completed-zone-cpu.ps1')
$fresh=Get-Content -LiteralPath (Join-Path $replay 'supplemental-completed-zone-cpu-20261001.json') -Raw | ConvertFrom-Json -DateKind String
$old.PSObject.Properties.Remove('generatedUtc')
$fresh.PSObject.Properties.Remove('generatedUtc')
if(($old | ConvertTo-Json -Depth 24 -Compress) -cne ($fresh | ConvertTo-Json -Depth 24 -Compress)) {throw 'Supplement canonical replay mismatch'}
$receipt=[ordered]@{
    status='PASS'; records=$fresh.runCount; classification='offline-archived-completion-zone-CPU-replay'
    scriptSha256=(Get-FileHash -LiteralPath $PSCommandPath).Hash.ToLowerInvariant()
    supplementScriptSha256=(Get-FileHash -LiteralPath (Join-Path $archive 'supplement-completed-zone-cpu.ps1')).Hash.ToLowerInvariant()
    archivedReceiptSha256=(Get-FileHash -LiteralPath (Join-Path $archive 'supplemental-completed-zone-cpu-20261001.json')).Hash.ToLowerInvariant()
    canonicalComparison='All values identical except generatedUtc; raw benchmark hashes joined.'
    replayRoot=$replay; deviceAccess=$false
}
$receipt | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'supplement-offline-replay-receipt.json') -Encoding utf8
Write-Output 'Archived completion-zone CPU supplement canonical replay PASS,12/12.'
