[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $repoRoot 'assets/models/player/runtime'
$expected = @{
    BodyPrimaryVisible = $true
    HeadPrimaryMasked = $false
    NearFacePrimaryMasked = $false
    GauntletPrimaryVisible = $true
}
foreach ($file in @('asset.manifest.json', 'clip-manifest.json')) {
    $manifest = Get-Content -LiteralPath (Join-Path $runtime $file) -Raw | ConvertFrom-Json
    $parts = @($manifest.primitiveSemantics)
    if ($parts.Count -ne 4) { throw "$file must declare exactly four primitive semantics" }
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($part in $parts) {
        if (@($expected.Keys | Where-Object { $_ -ceq $part.material }).Count -ne 1 -or
            -not $seen.Add($part.material)) { throw "$file has an unknown or duplicate semantic" }
        foreach ($field in @('firstPersonPrimary', 'shadow', 'reflection')) {
            if ($part.$field -isnot [bool]) { throw "$file $($part.material) needs a Boolean $field" }
        }
        if ($part.firstPersonPrimary -ne $expected[$part.material] -or
            -not $part.shadow -or -not $part.reflection) {
            throw "$file $($part.material) contradicts the player visibility contract"
        }
    }
}
$atlas = Get-Content -LiteralPath (Join-Path $repoRoot 'assets/textures/props/runtime/asset.manifest.json') -Raw | ConvertFrom-Json
$processing = Get-Content -LiteralPath (Join-Path $runtime 'gothic-traveller-lod0.runtime.glb.processing.json') -Raw | ConvertFrom-Json
if ($processing.runtime -cne 'gothic-traveller-lod0.runtime.glb' -or $processing.exportThreads -ne 1 -or
    (Get-FileHash -LiteralPath (Join-Path $runtime $processing.runtime) -Algorithm SHA256).Hash.ToLowerInvariant() -cne $processing.runtimeSha256) {
    throw 'Admitted player runtime must match its single-thread generation receipt'
}
$generatedParts = @($processing.primitiveSemantics.PSObject.Properties)
if ($generatedParts.Count -ne 4 -or
    ($generatedParts.Value | Measure-Object -Sum).Sum -ne $processing.triangles) {
    throw 'Player processing receipt must account for all four primitive triangle counts'
}
foreach ($part in $generatedParts) {
    if (@($expected.Keys | Where-Object { $_ -ceq $part.Name }).Count -ne 1 -or $part.Value -le 0) {
        throw 'Player processor and manifests disagree on nonempty named parts'
    }
}
if ($atlas.layerOrder[2] -cne 'gothic-traveller-lod0' -or
    $atlas.layerOrder[3] -cne 'gothic-traveller-lod0.GauntletPrimaryVisible') {
    throw 'Generated production atlas must match tested Body/Gauntlet layers after sword/torch'
}
Write-Output 'Player runtime/clip manifests and generated atlas order agree with the four-way contract.'
