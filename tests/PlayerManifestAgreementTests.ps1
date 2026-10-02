[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $repoRoot 'assets/models/player/runtime'
$viewRuntime = Join-Path $repoRoot 'assets/models/player/viewmodel/runtime'
$worldHash = '46a88dac9dd569117be5a17a35540fecf3e0e6026d3ad404dd5fdbc85a9ad54f'
$worldManifestHash = '556f8b4f4f7509ee9cb6d8350d5bb79dd430a7544d923d88051794f18ab61141'
$viewHash = 'eaa0db3abb8ff2dae616247054c190031a08c542dd95396cc6dc1e18a7462488'

$worldExpected = @{
    BodyPrimaryVisible = $true
    GauntletPrimaryVisible = $true
    HeadPrimaryMasked = $false
    NearFacePrimaryMasked = $false
    BodyRemainderPrimaryVisible = $true
}
$viewExpected = @{
    ViewmodelSleeves = $true
    ViewmodelGauntlets = $true
}

function Assert-PrimitiveSemantics {
    param(
        [Parameter(Mandatory = $true)]$Document,
        [Parameter(Mandatory = $true)][hashtable]$Expected,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][bool]$Shadow,
        [Parameter(Mandatory = $true)][bool]$Reflection
    )
    $parts = @($Document.primitiveSemantics)
    if ($parts.Count -ne $Expected.Count) { throw "$Name must declare exactly $($Expected.Count) primitive semantics" }
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($part in $parts) {
        if (-not $Expected.ContainsKey($part.material) -or -not $seen.Add($part.material)) {
            throw "$Name has an unknown or duplicate semantic: $($part.material)"
        }
        foreach ($field in @('firstPersonPrimary', 'shadow', 'reflection')) {
            if ($part.$field -isnot [bool]) { throw "$Name $($part.material) needs a Boolean $field" }
        }
        if ($part.firstPersonPrimary -ne $Expected[$part.material] -or
            $part.shadow -ne $Shadow -or $part.reflection -ne $Reflection) {
            throw "$Name $($part.material) contradicts its visibility, shadow or reflection contract"
        }
    }
    foreach ($material in $Expected.Keys) {
        if (-not $seen.Contains($material)) { throw "$Name is missing semantic $material" }
    }
}

function Get-Sha256([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

$worldManifestPath = Join-Path $runtime 'asset.manifest.json'
$clipManifestPath = Join-Path $runtime 'clip-manifest.json'
$worldPath = Join-Path $runtime 'gothic-traveller-lod0.runtime.glb'
$worldReceiptPath = "$worldPath.processing.json"
$worldManifest = Get-Content -LiteralPath $worldManifestPath -Raw | ConvertFrom-Json
$clipManifest = Get-Content -LiteralPath $clipManifestPath -Raw | ConvertFrom-Json
$worldReceipt = Get-Content -LiteralPath $worldReceiptPath -Raw | ConvertFrom-Json

if ($worldManifest.playerAssetRole -cne 'WorldBody' -or
    $worldManifest.distribution -cne 'runtime' -or
    $worldManifest.budgets.maxPrimitives -ne 5 -or
    $worldManifest.budgets.maxMaterials -ne 5 -or
    $worldManifest.budgets.maxTextureLayersPerKind -ne 5) {
    throw 'World manifest must admit the accepted five-region WorldBody runtime asset.'
}
Assert-PrimitiveSemantics $worldManifest $worldExpected 'World manifest' $true $true
Assert-PrimitiveSemantics $clipManifest $worldExpected 'Clip manifest' $true $true

if ($worldReceipt.runtime -cne 'gothic-traveller-lod0.runtime.glb' -or
    $worldReceipt.exportThreads -ne 1 -or
    $worldReceipt.runtimeSha256 -cne $worldHash -or
    $worldReceipt.manifestSha256 -cne $worldManifestHash -or
    (Get-Sha256 $worldPath) -cne $worldHash -or
    (Get-Sha256 $worldManifestPath) -cne $worldManifestHash) {
    throw 'Accepted world GLB/manifest must match the exact single-thread promotion receipt hashes.'
}
$worldParts = @($worldReceipt.primitiveSemantics.PSObject.Properties)
$worldExpectedTriangles = 27815
if ($worldReceipt.vertices -ne 34304 -or $worldReceipt.triangles -ne $worldExpectedTriangles -or
    $worldParts.Count -ne $worldExpected.Count -or
    ($worldParts.Value | Measure-Object -Sum).Sum -ne $worldExpectedTriangles) {
    throw 'World processing receipt must account for all five accepted primitive counts and vertices.'
}
foreach ($part in $worldParts) {
    if (-not $worldExpected.ContainsKey($part.Name) -or $part.Value -le 0) {
        throw "World processing receipt has an unknown or empty primitive: $($part.Name)"
    }
}

$viewManifestPath = Join-Path $viewRuntime 'asset.manifest.json'
$viewPath = Join-Path $viewRuntime 'gothic-traveller-viewmodel.runtime.glb'
$viewReceiptPath = Join-Path $viewRuntime 'viewmodel-processing.json'
$viewManifest = Get-Content -LiteralPath $viewManifestPath -Raw | ConvertFrom-Json
$viewReceipt = Get-Content -LiteralPath $viewReceiptPath -Raw | ConvertFrom-Json
if ($viewManifest.playerAssetRole -cne 'Viewmodel' -or $viewManifest.distribution -cne 'runtime') {
    throw 'Viewmodel manifest must identify the accepted runtime Viewmodel asset.'
}
Assert-PrimitiveSemantics $viewManifest $viewExpected 'Viewmodel manifest' $false $false
if ($viewReceipt.runtimeSha256 -cne $viewHash -or (Get-Sha256 $viewPath) -cne $viewHash -or
    $viewReceipt.sourceWorldSha256 -cne 'e8737f10e7669b284e04109d9c3acdf537df284093a21511656bf191a70450fd' -or
    $viewReceipt.pairedWorldSha256 -cne $worldHash -or
    $viewReceipt.pairedWorldManifestSha256 -cne $worldManifestHash -or
    $viewReceipt.pairedWorldRepositoryPath -cne 'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb' -or
    $viewReceipt.pairedWorldManifestRepositoryPath -cne 'assets/models/player/runtime/asset.manifest.json') {
    throw 'Accepted viewmodel must match its own hash, preserved base-rig authority and paired world hashes/paths.'
}
$viewParts = @($viewReceipt.primitiveSemantics.PSObject.Properties)
if ($viewReceipt.gauntletSourceHandedness -cne 'Right' -or
    $viewReceipt.gauntletScale -ne 0.099 -or
    $viewReceipt.rightGauntletCuffFit.proximalVertices -ne 2023 -or
    $viewReceipt.rightGauntletCuffFit.distalHandVerticesPreserved -ne 3587 -or
    $viewReceipt.rightGauntletCuffFit.radialCorrectionMetres -le 0 -or
    $viewReceipt.rightGauntletCuffFit.radialCorrectionMetres -gt 0.08) {
    throw 'Right cuff receipt must preserve handedness, scale and the guarded local fit/distal hand roster.'
}
if ($viewParts.Count -ne 2 -or ($viewParts.Value | Measure-Object -Sum).Sum -ne 14370) {
    throw 'Viewmodel processing receipt must account for exactly the two accepted viewmodel regions.'
}
foreach ($part in $viewParts) {
    if (-not $viewExpected.ContainsKey($part.Name) -or $part.Value -le 0) {
        throw "Viewmodel receipt has an unknown or empty primitive: $($part.Name)"
    }
}

$atlas = Get-Content -LiteralPath (Join-Path $repoRoot 'assets/textures/props/runtime/asset.manifest.json') -Raw | ConvertFrom-Json
if ($atlas.layerOrder[2] -cne 'gothic-traveller-lod0' -or
    $atlas.layerOrder[3] -cne 'gothic-traveller-lod0.GauntletPrimaryVisible' -or
    @($atlas.layerOrder | Where-Object { $_ -ceq 'gothic-traveller-lod0.BodyRemainderPrimaryVisible' }).Count -ne 0) {
    throw 'WorldBody and Gauntlet must keep the existing shared Body/Gauntlet atlas layers; BodyRemainder adds no atlas layer.'
}
Write-Output 'Accepted five-region WorldBody and two-region Viewmodel manifests, receipts, hashes and shared atlas contract agree.'
