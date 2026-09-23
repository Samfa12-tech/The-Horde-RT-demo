[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Scanner,
    [Parameter(Mandatory = $true)][string]$StrippedLibraryPath,
    [Parameter(Mandatory = $true)][string]$ApkPath,
    [Parameter(Mandatory = $true)][ValidateSet('Shipping','Diagnostic')][string]$Instrumentation,
    [Parameter(Mandatory = $true)][ValidateSet('Mobile','High')][string]$Quality,
    [switch]$SkipExternalValidation
)

$ErrorActionPreference = 'Stop'
function Assert-True([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

$resolvedScanner = (Resolve-Path -LiteralPath $Scanner -ErrorAction Stop).Path
$resolvedLibrary = (Resolve-Path -LiteralPath $StrippedLibraryPath -ErrorAction Stop).Path
$resolvedApk = (Resolve-Path -LiteralPath $ApkPath -ErrorAction Stop).Path
$entryName = 'lib/arm64-v8a/libhorde_rt_probe_android.so'
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) (
    'horde-rt-android-containment-' + [guid]::NewGuid().ToString('N'))
$archive = $null
try {
    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead($resolvedApk)
    $entries = @($archive.Entries | Where-Object { $_.FullName -ceq $entryName })
    Assert-True ($entries.Count -eq 1) `
        "Android APK must contain exactly one $entryName entry; observed $($entries.Count)."
    $extractedPath = Join-Path $temporaryRoot 'libhorde_rt_probe_android.so'
    $input = $entries[0].Open()
    try {
        $output = [IO.File]::Create($extractedPath)
        try { $input.CopyTo($output) }
        finally { $output.Dispose() }
    }
    finally { $input.Dispose() }
    $archive.Dispose()
    $archive = $null

    $strippedHash = (Get-FileHash -LiteralPath $resolvedLibrary -Algorithm SHA256).Hash.ToLowerInvariant()
    $packagedHash = (Get-FileHash -LiteralPath $extractedPath -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-True ($strippedHash -ceq $packagedHash) `
        'The stripped ARM64 library and exact APK entry do not agree byte-for-byte.'

    if ($SkipExternalValidation) {
        $strippedSummary = (& $resolvedScanner -TargetPath $resolvedLibrary `
            -TargetPlatform Android -Instrumentation $Instrumentation -Quality $Quality `
            -SkipExternalValidation | Out-String).Trim()
        $packagedSummary = (& $resolvedScanner -TargetPath $extractedPath `
            -TargetPlatform Android -Instrumentation $Instrumentation -Quality $Quality `
            -SkipExternalValidation | Out-String).Trim()
    } else {
        $strippedSummary = (& $resolvedScanner -TargetPath $resolvedLibrary `
            -TargetPlatform Android -Instrumentation $Instrumentation -Quality $Quality |
            Out-String).Trim()
        $packagedSummary = (& $resolvedScanner -TargetPath $extractedPath `
            -TargetPlatform Android -Instrumentation $Instrumentation -Quality $Quality |
            Out-String).Trim()
    }
    $strippedResult = $strippedSummary | ConvertFrom-Json
    $packagedResult = $packagedSummary | ConvertFrom-Json
    Assert-True ($strippedResult.targetSha256 -ceq $packagedResult.targetSha256 -and
                 $strippedResult.targetSha256 -ceq $strippedHash) `
        'Android containment summaries disagree with the byte-identical package subjects.'

    [pscustomobject]@{
        apk = $resolvedApk
        apkEntry = $entryName
        instrumentation = $Instrumentation
        quality = $Quality
        arm64Sha256 = $strippedHash
        stripped = $strippedResult
        packaged = $packagedResult
    } | ConvertTo-Json -Depth 8 -Compress
}
finally {
    if ($null -ne $archive) { $archive.Dispose() }
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
