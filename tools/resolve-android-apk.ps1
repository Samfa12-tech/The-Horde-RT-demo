param(
    [Parameter(Mandatory=$true)][string]$AndroidRoot,
    [ValidateSet('debug', 'release', 'benchmark')][string]$Variant = 'debug'
)
$ErrorActionPreference = 'Stop'
$buildRoot = [IO.Path]::GetFullPath((Join-Path $AndroidRoot 'app/build'))
$capitalized = $Variant.Substring(0, 1).ToUpperInvariant() + $Variant.Substring(1)
$redirect = Join-Path $buildRoot "intermediates/apk_ide_redirect_file/$Variant/create${capitalized}ApkListingFileRedirect/redirect.txt"
$listingLines = @(Get-Content -LiteralPath $redirect | Where-Object { $_.StartsWith('listingFile=') })
if ($listingLines.Count -ne 1) { throw 'Gradle APK redirect must identify exactly one listing.' }
$listing = [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $redirect) $listingLines[0].Substring(12)))
$buildPrefix = $buildRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $listing.StartsWith($buildPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Gradle APK listing escaped the application build directory.'
}
$metadata = Get-Content -LiteralPath $listing -Raw | ConvertFrom-Json
if ($metadata.variantName -cne $Variant -or $metadata.artifactType.type -cne 'APK' -or
    @($metadata.elements).Count -ne 1) {
    throw 'Expected one APK for the requested Gradle variant.'
}
$name = [string]$metadata.elements[0].outputFile
if (-not $name -or [IO.Path]::GetFileName($name) -cne $name -or $name.Contains('/') -or $name.Contains('\')) {
    throw 'Gradle APK output must be a plain file name.'
}
$apk = Join-Path (Split-Path -Parent $listing) $name
if (-not (Test-Path -LiteralPath $apk -PathType Leaf)) { throw 'The APK named by Gradle is missing.' }
$apk
