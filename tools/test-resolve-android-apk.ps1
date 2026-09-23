param()

$ErrorActionPreference = 'Stop'
$resolver = Join-Path $PSScriptRoot 'resolve-android-apk.ps1'
if (-not (Test-Path -LiteralPath $resolver -PathType Leaf)) {
    throw "Resolver not found: $resolver"
}

$tempParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd([IO.Path]::DirectorySeparatorChar)
$testRoot = Join-Path $tempParent ("resolve-apk-tests-" + [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $testRoot)
$passed = 0

function Assert-Equal {
    param([string]$Expected, [string]$Actual, [string]$Message)
    if ($Expected -cne $Actual) {
        throw "$Message`nExpected: $Expected`nActual:   $Actual"
    }
}

function New-Fixture {
    param([string]$Name, [string]$Variant = 'debug')
    $root = Join-Path $testRoot ($Name + '-' + [guid]::NewGuid().ToString('N'))
    $build = Join-Path $root 'app/build'
    $capitalized = $Variant.Substring(0, 1).ToUpperInvariant() + $Variant.Substring(1)
    $redirectDir = Join-Path $build "intermediates/apk_ide_redirect_file/$Variant/create${capitalized}ApkListingFileRedirect"
    [void](New-Item -ItemType Directory -Path $redirectDir -Force)
    [pscustomobject]@{ Root = $root; Build = $build; Redirect = Join-Path $redirectDir 'redirect.txt'; RedirectDir = $redirectDir }
}

function Write-Listing {
    param(
        [pscustomobject]$Fixture,
        [string]$ListingPath,
        [string]$Variant = 'debug',
        [string]$Type = 'APK',
        [object[]]$Elements = @([pscustomobject]@{ outputFile = 'app-debug.apk' }),
        [switch]$CreateApk
    )
    $listingPath = [IO.Path]::GetFullPath($ListingPath)
    [void](New-Item -ItemType Directory -Path (Split-Path -Parent $listingPath) -Force)
    $metadata = [pscustomobject]@{
        variantName = $Variant
        artifactType = [pscustomobject]@{ type = $Type }
        elements = @($Elements)
    }
    $metadata | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $listingPath -Encoding UTF8
    if ($CreateApk) {
        foreach ($element in @($Elements)) {
            if ($element.outputFile -and [IO.Path]::GetFileName([string]$element.outputFile) -ceq [string]$element.outputFile) {
                [IO.File]::WriteAllBytes((Join-Path (Split-Path -Parent $listingPath) $element.outputFile), [byte[]]@())
            }
        }
    }
    $listingPath
}

function Set-RedirectTo {
    param([pscustomobject]$Fixture, [string]$ListingPath, [string[]]$ExtraLines = @())
    $relative = [IO.Path]::GetRelativePath($Fixture.RedirectDir, $ListingPath)
    @("listingFile=$relative") + $ExtraLines | Set-Content -LiteralPath $Fixture.Redirect -Encoding UTF8
}

function Assert-Resolves {
    param([pscustomobject]$Fixture, [string]$ExpectedApk, [string]$Variant = 'debug', [string]$Name)
    $actual = @(& $resolver -AndroidRoot $Fixture.Root -Variant $Variant)
    if ($actual.Count -ne 1) { throw "$Name returned $($actual.Count) values; expected one APK path." }
    Assert-Equal ([IO.Path]::GetFullPath($ExpectedApk)) ([IO.Path]::GetFullPath([string]$actual[0])) "$Name resolved the wrong APK."
    $script:passed++
    Write-Output "PASS $Name"
}

function Assert-ResolverFails {
    param([pscustomobject]$Fixture, [string]$Variant = 'debug', [string]$Name)
    $failed = $false
    try {
        $null = @(& $resolver -AndroidRoot $Fixture.Root -Variant $Variant)
    }
    catch {
        $failed = $true
    }
    if (-not $failed) { throw "$Name unexpectedly succeeded." }
    $script:passed++
    Write-Output "PASS $Name (rejected)"
}

try {
    # Standard AGP layout: redirect points back to outputs/apk/<variant>.
    $fixture = New-Fixture 'normal'
    $listing = Write-Listing $fixture (Join-Path $fixture.Build 'outputs/apk/debug/output-metadata.json') -CreateApk
    Set-RedirectTo $fixture $listing
    Assert-Resolves $fixture (Join-Path (Split-Path -Parent $listing) 'app-debug.apk') 'debug' 'normal Gradle output'

    # Gradle can place both its listing and APK in an intermediates directory.
    $fixture = New-Fixture 'redirected'
    $listing = Write-Listing $fixture (Join-Path $fixture.Build 'intermediates/packaged_res/debug/output-metadata.json') -CreateApk
    Set-RedirectTo $fixture $listing
    Assert-Resolves $fixture (Join-Path (Split-Path -Parent $listing) 'app-debug.apk') 'debug' 'redirected intermediates output'

    $fixture = New-Fixture 'wrong-variant'
    $listing = Write-Listing $fixture (Join-Path $fixture.Build 'outputs/apk/debug/output-metadata.json') -Variant 'release' -CreateApk
    Set-RedirectTo $fixture $listing
    Assert-ResolverFails $fixture 'debug' 'wrong variant'

    $fixture = New-Fixture 'wrong-type'
    $listing = Write-Listing $fixture (Join-Path $fixture.Build 'outputs/apk/debug/output-metadata.json') -Type 'AAB' -CreateApk
    Set-RedirectTo $fixture $listing
    Assert-ResolverFails $fixture 'debug' 'wrong artifact type'

    $fixture = New-Fixture 'missing-elements'
    $listing = Write-Listing $fixture (Join-Path $fixture.Build 'outputs/apk/debug/output-metadata.json') -Elements @()
    Set-RedirectTo $fixture $listing
    Assert-ResolverFails $fixture 'debug' 'missing output element'

    $fixture = New-Fixture 'multiple-elements'
    $elements = @(
        [pscustomobject]@{ outputFile = 'app-debug.apk' },
        [pscustomobject]@{ outputFile = 'app-debug-universal.apk' }
    )
    $listing = Write-Listing $fixture (Join-Path $fixture.Build 'outputs/apk/debug/output-metadata.json') -Elements $elements -CreateApk
    Set-RedirectTo $fixture $listing
    Assert-ResolverFails $fixture 'debug' 'multiple output elements'

    $fixture = New-Fixture 'missing-apk'
    $listing = Write-Listing $fixture (Join-Path $fixture.Build 'outputs/apk/debug/output-metadata.json')
    Set-RedirectTo $fixture $listing
    Assert-ResolverFails $fixture 'debug' 'missing APK file'

    $fixture = New-Fixture 'traversal-name'
    $listing = Write-Listing $fixture (Join-Path $fixture.Build 'outputs/apk/debug/output-metadata.json') -Elements @([pscustomobject]@{ outputFile = '../outside.apk' })
    Set-RedirectTo $fixture $listing
    Assert-ResolverFails $fixture 'debug' 'filename path traversal'

    $fixture = New-Fixture 'escaped-listing'
    $outsideListing = Join-Path $testRoot 'outside-listing.json'
    $listing = Write-Listing $fixture $outsideListing -CreateApk
    Set-RedirectTo $fixture $listing
    Assert-ResolverFails $fixture 'debug' 'escaped listing path'

    $fixture = New-Fixture 'duplicate-redirect'
    $listing = Write-Listing $fixture (Join-Path $fixture.Build 'outputs/apk/debug/output-metadata.json') -CreateApk
    Set-RedirectTo $fixture $listing @('listingFile=another-listing.json')
    Assert-ResolverFails $fixture 'debug' 'duplicate redirect entries'

    Write-Output "All $passed resolver cases passed."
}
finally {
    # Delete only this run's GUID-named fixture directory after validating its exact temp location.
    $resolvedRoot = [IO.Path]::GetFullPath($testRoot)
    if ($resolvedRoot.StartsWith($tempParent + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $resolvedRoot) -match '^resolve-apk-tests-[0-9a-f]{32}$') {
        Remove-Item -LiteralPath $resolvedRoot -Recurse -Force
    }
}
