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
Write-Output 'Player runtime/clip manifests agree with the four-way visibility contract.'
