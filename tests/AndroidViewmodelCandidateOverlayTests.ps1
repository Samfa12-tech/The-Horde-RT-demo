[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$CandidateDirectory)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$androidRoot = Join-Path $repoRoot 'android'
$candidateSource = [IO.Path]::GetFullPath($CandidateDirectory)
$overlayRoot = [IO.Path]::GetFullPath((Join-Path $androidRoot 'app/build/generated/viewmodelCandidateAssets'))
$generatedRoot = [IO.Path]::GetFullPath((Join-Path $androidRoot 'app/build/generated'))
$temporaryBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$temporaryRoot = Join-Path $temporaryBase ('horde-android-viewmodel-overlay-' + [guid]::NewGuid().ToString('N'))
$originalOverlay = Join-Path $temporaryRoot 'original-overlay'
$hadOriginalOverlay = Test-Path -LiteralPath $overlayRoot -PathType Container
$snapshotReady = $false
$gradle = Join-Path $androidRoot 'gradlew.bat'

if (-not (Test-Path -LiteralPath $gradle -PathType Leaf)) { throw 'Gradle wrapper is missing.' }
if (-not (Test-Path -LiteralPath $candidateSource -PathType Container)) { throw "Candidate directory is missing: $candidateSource" }
$sourceReceiptPath = Join-Path $candidateSource 'viewmodel-processing.json'
if (-not (Test-Path -LiteralPath $sourceReceiptPath -PathType Leaf)) { throw 'Candidate processing receipt is missing.' }
$sourceReceipt = Get-Content -LiteralPath $sourceReceiptPath -Raw | ConvertFrom-Json
if (-not $sourceReceipt.pairedWorldManifest -or -not $sourceReceipt.pairedWorldManifestSha256) {
    throw 'This test requires a candidate receipt with the optional paired-world manifest fields.'
}

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Write-Receipt([string]$Directory, [object]$Receipt) {
    $json = ConvertTo-Json -InputObject $Receipt -Depth 100
    [IO.File]::WriteAllText((Join-Path $Directory 'viewmodel-processing.json'), $json, [Text.UTF8Encoding]::new($false))
}

function New-CandidateVariant([string]$Name, [scriptblock]$MutateReceipt) {
    $directory = Join-Path $temporaryRoot $Name
    New-Item -ItemType Directory -Path $directory | Out-Null
    $receipt = Get-Content -LiteralPath $sourceReceiptPath -Raw | ConvertFrom-Json
    $names = @($receipt.pairedWorldRuntime, $receipt.runtime, $receipt.pairedWorldManifest)
    foreach ($name in $names) {
        if ([string]::IsNullOrWhiteSpace([string]$name) -or [IO.Path]::GetFileName([string]$name) -cne [string]$name) {
            throw "Candidate receipt contains a nonlocal file name: $name"
        }
        Copy-Item -LiteralPath (Join-Path $candidateSource ([string]$name)) -Destination $directory
    }
    & $MutateReceipt $receipt
    Write-Receipt $directory $receipt
    return $directory
}

function Invoke-OverlayTask(
    [string]$Directory,
    [bool]$ExpectedPass,
    [string]$CaseName,
    [string]$ExpectedDiagnostic = '',
    [string[]]$ExtraArguments = @()) {
    Push-Location $androidRoot
    try {
        $output = & $gradle ':app:prepareViewmodelCandidateAssets' "-PhordeViewmodelCandidateDir=$Directory" '--console=plain' @ExtraArguments 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    if ($ExpectedPass -and $exitCode -ne 0) {
        throw "$CaseName unexpectedly failed (exit $exitCode):`n$($output | Out-String)"
    }
    if (-not $ExpectedPass -and $exitCode -eq 0) {
        throw "$CaseName unexpectedly passed."
    }
    if (-not $ExpectedPass -and
        ([string]::IsNullOrWhiteSpace($ExpectedDiagnostic) -or
         (($output | Out-String) -notmatch [regex]::Escape($ExpectedDiagnostic)))) {
        throw "$CaseName failed without the expected diagnostic '$ExpectedDiagnostic':`n$($output | Out-String)"
    }
    Write-Output ("{0}: {1}" -f $CaseName, $(if ($ExpectedPass) { 'passed' } else { 'rejected' }))
}

try {
    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    if ($hadOriginalOverlay) {
        Copy-Item -LiteralPath $overlayRoot -Destination $originalOverlay -Recurse
    }
    $snapshotReady = $true

    Invoke-OverlayTask $candidateSource $true 'paired manifest stages'
    Invoke-OverlayTask $candidateSource $false 'retired legacy mounting rejects' `
        'Legacy player mounting is retired; production uses the accepted anatomical profile.' `
        @('-PhordeAnatomicalPlayerMount=false')
    Invoke-OverlayTask $candidateSource $false 'malformed anatomical profile selector rejects' `
        'hordeAnatomicalPlayerMount must be true or false.' `
        @('-PhordeAnatomicalPlayerMount=invalid')
    Invoke-OverlayTask $candidateSource $true 'explicit anatomical candidate stages' '' `
        @('-PhordeAnatomicalPlayerMount=true')
    $stagedManifest = Join-Path $overlayRoot 'models/player/runtime/asset.manifest.json'
    if (-not (Test-Path -LiteralPath $stagedManifest -PathType Leaf) -or
        (Get-Sha256 $stagedManifest) -cne ([string]$sourceReceipt.pairedWorldManifestSha256).ToLowerInvariant()) {
        throw 'Paired world manifest was not staged at the world runtime destination with the receipted hash.'
    }
    $stagedViewReceipt = Join-Path $overlayRoot 'models/player/viewmodel/runtime/candidate-receipt.json'
    if (-not (Test-Path -LiteralPath $stagedViewReceipt -PathType Leaf)) {
        throw 'The existing viewmodel candidate receipt staging path changed.'
    }

    $missingHash = New-CandidateVariant 'missing-manifest-hash' {
        param($receipt)
        $receipt.PSObject.Properties.Remove('pairedWorldManifestSha256')
    }
    Invoke-OverlayTask $missingHash $false 'manifest name without hash rejects' `
        'Candidate paired-world manifest name and SHA-256 must be supplied together.'

    $hashOnly = New-CandidateVariant 'manifest-hash-only' {
        param($receipt)
        $receipt.PSObject.Properties.Remove('pairedWorldManifest')
    }
    Invoke-OverlayTask $hashOnly $false 'manifest hash without name rejects' `
        'Candidate paired-world manifest name and SHA-256 must be supplied together.'

    $wrongHash = New-CandidateVariant 'wrong-manifest-hash' {
        param($receipt)
        $receipt.pairedWorldManifestSha256 = ('0' * 64)
    }
    Invoke-OverlayTask $wrongHash $false 'manifest hash mismatch rejects' `
        'Viewmodel candidate role, handedness, hashes or source authority disagree.'

    $traversal = New-CandidateVariant 'manifest-path-traversal' {
        param($receipt)
        $receipt.pairedWorldManifest = '..\asset.manifest.json'
    }
    Invoke-OverlayTask $traversal $false 'manifest path traversal rejects' `
        'Candidate receipt requires plain local file names.'

    $legacy = New-CandidateVariant 'legacy-four-field-receipt' {
        param($receipt)
        $receipt.PSObject.Properties.Remove('pairedWorldManifest')
        $receipt.PSObject.Properties.Remove('pairedWorldManifestSha256')
    }
    Invoke-OverlayTask $legacy $false 'missing paired manifest rejects by default' `
        'Modelled viewmodel candidates require an explicit paired-world manifest.'
    Invoke-OverlayTask $legacy $false 'anatomical profile without paired manifest rejects' `
        'Modelled viewmodel candidates require an explicit paired-world manifest.' `
        @('-PhordeAnatomicalPlayerMount=true')
    if (-not (Test-Path -LiteralPath $stagedManifest) -or
        (Get-Sha256 $stagedManifest) -cne ([string]$sourceReceipt.pairedWorldManifestSha256).ToLowerInvariant()) {
        throw 'Rejected candidate must not remove or alter the valid paired world-manifest overlay.'
    }
} finally {
    $resolvedOverlay = [IO.Path]::GetFullPath($overlayRoot)
    $expectedOverlayPrefix = $generatedRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (-not $resolvedOverlay.StartsWith($expectedOverlayPrefix, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedOverlay) -cne 'viewmodelCandidateAssets') {
        throw 'Refused to restore an overlay outside the exact generated candidate directory.'
    }
    if ($snapshotReady) {
        if (Test-Path -LiteralPath $resolvedOverlay) {
            Remove-Item -LiteralPath $resolvedOverlay -Recurse -Force
        }
        if ($hadOriginalOverlay -and (Test-Path -LiteralPath $originalOverlay -PathType Container)) {
            Copy-Item -LiteralPath $originalOverlay -Destination $resolvedOverlay -Recurse
        }
    }
    $resolvedTemporaryRoot = [IO.Path]::GetFullPath($temporaryRoot)
    if ((Test-Path -LiteralPath $resolvedTemporaryRoot -PathType Container) -and
        [IO.Directory]::GetParent($resolvedTemporaryRoot).FullName -eq $temporaryBase.TrimEnd('\', '/') -and
        [IO.Path]::GetFileName($resolvedTemporaryRoot).StartsWith('horde-android-viewmodel-overlay-')) {
        Remove-Item -LiteralPath $resolvedTemporaryRoot -Recurse -Force
    } else {
        throw 'Refused unsafe candidate overlay fixture cleanup.'
    }
}
