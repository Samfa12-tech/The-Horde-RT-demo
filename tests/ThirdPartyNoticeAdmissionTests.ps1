param([string]$WindowsZip = '', [string]$AndroidApk = '')
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $repoRoot 'tools/third-party-notice-policy.ps1')
Add-Type -AssemblyName System.IO.Compression.FileSystem
$scratchRoot = Join-Path ([IO.Path]::GetTempPath()) ('Horde-ThirdPartyNotices-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $scratchRoot | Out-Null
$cases = 0
function New-NoticeFixture([string]$Name, [string]$EntryName, [byte[]]$Bytes, [int]$Copies = 1) {
    $path = Join-Path $scratchRoot $Name
    $zip = [IO.Compression.ZipFile]::Open($path, [IO.Compression.ZipArchiveMode]::Create)
    try {
        for ($i = 0; $i -lt $Copies; ++$i) {
            $entry = $zip.CreateEntry($EntryName)
            $stream = $entry.Open()
            try { $stream.Write($Bytes, 0, $Bytes.Length) } finally { $stream.Dispose() }
        }
    } finally { $zip.Dispose() }
    return $path
}
function Assert-Rejected([string]$Path, [string]$Platform) {
    $rejected = $false
    try { Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $Path -Platform $Platform }
    catch { $rejected = $true }
    if (-not $rejected) { throw "Invalid $Platform notice fixture passed admission: $Path" }
}
try {
    # Exercise the same staging function used by Windows candidate packaging.
    $stage = Join-Path $scratchRoot 'stage'
    Copy-HordeThirdPartyNotices -RepositoryRoot $repoRoot -PackageRoot $stage
    $notice = [IO.File]::ReadAllBytes((Join-Path $stage 'THIRD_PARTY_NOTICES/cgltf-LICENSE.txt'))
    $stagedZip = Join-Path $scratchRoot 'windows-staged.zip'
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $stagedZip
    Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $stagedZip -Platform Windows
    ++$cases
    foreach ($platform in @('Windows', 'Android')) {
        $entryName = 'THIRD_PARTY_NOTICES/cgltf-LICENSE.txt'
        if ($platform -eq 'Android') { $entryName = 'assets/' + $entryName }
        $valid = New-NoticeFixture "$platform-valid.zip" $entryName $notice
        Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $valid -Platform $platform
        ++$cases
        $missing = New-NoticeFixture "$platform-missing.zip" $entryName $notice 0
        Assert-Rejected $missing $platform
        ++$cases
        $truncated = New-NoticeFixture "$platform-truncated.zip" $entryName $notice[0..100]
        Assert-Rejected $truncated $platform
        ++$cases
        $changed = [byte[]]$notice.Clone()
        $changed[$changed.Length - 1] = $changed[$changed.Length - 1] -bxor 1
        $corrupt = New-NoticeFixture "$platform-corrupt.zip" $entryName $changed
        Assert-Rejected $corrupt $platform
        ++$cases
        $duplicate = New-NoticeFixture "$platform-duplicate.zip" $entryName $notice 2
        Assert-Rejected $duplicate $platform
        ++$cases
        $wrongPath = New-NoticeFixture "$platform-wrong-path.zip" "wrong/$entryName" $notice
        Assert-Rejected $wrongPath $platform
        ++$cases
    }
    # Keep actual package admission hooked up, including the Android debug APK.
    $package = Get-Content (Join-Path $repoRoot 'tools/package-alpha.ps1') -Raw
    foreach ($call in @(
        'Copy-HordeThirdPartyNotices -RepositoryRoot $repoRoot -PackageRoot $windowsStage',
        'Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $windowsZip -Platform Windows',
        'Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $debugCandidate -Platform Android',
        'Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $androidCandidate -Platform Android')) {
        if (-not $package.Contains($call)) { throw "Candidate packaging lost notice admission: $call" }
    }
    $foundation = Get-Content (Join-Path $repoRoot 'tools/run-foundation-validation.ps1') -Raw
    foreach ($call in @(
        'Copy-HordeThirdPartyNotices -RepositoryRoot $repoRoot -PackageRoot $windowsStage',
        'Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $windowsZip -Platform Windows',
        'Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $androidValidationApk -Platform Android')) {
        if (-not $foundation.Contains($call)) { throw "Validation packaging lost notice admission: $call" }
    }
    foreach ($relative in @('src/platform/windows/DiagnosticWindow.cpp', 'android/app/src/main/res/values/strings.xml')) {
        $credits = Get-Content (Join-Path $repoRoot $relative) -Raw
        foreach ($marker in @('cgltf by Johannes Kuhlmann', 'THIRD_PARTY_NOTICES/cgltf-LICENSE.txt')) {
            if (-not $credits.Contains($marker)) { throw "$relative lost cgltf notice credits: $marker" }
        }
    }
    $gradle = Get-Content (Join-Path $repoRoot 'android/app/build.gradle') -Raw
    if ($gradle -notmatch "(?s)from\('../../third_party/cgltf'\)\s*\{\s*include 'LICENSE'\s*rename 'LICENSE', 'cgltf-LICENSE.txt'\s*into 'THIRD_PARTY_NOTICES'\s*\}") {
        throw 'Android runtime-assets Sync must copy the full cgltf notice to the admitted APK path.'
    }
    foreach ($relative in @('tools/package-alpha.ps1', 'tools/run-foundation-validation.ps1', 'tools/third-party-notice-policy.ps1')) {
        $errors = $null
        $null = [Management.Automation.Language.Parser]::ParseFile((Join-Path $repoRoot $relative), [ref]$null, [ref]$errors)
        if ($errors.Count) { throw "$relative does not parse: $errors" }
    }
    if ($WindowsZip) { Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $WindowsZip -Platform Windows }
    if ($AndroidApk) { Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $AndroidApk -Platform Android }
    Write-Output "Third-party notice admission passed $cases archive fixtures and candidate/Android staging hooks."
} finally {
    $resolvedScratch = [IO.Path]::GetFullPath($scratchRoot)
    $tempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (-not $resolvedScratch.StartsWith($tempPrefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture cleanup escaped the temporary root.' }
    Remove-Item -LiteralPath $resolvedScratch -Recurse -Force
}
