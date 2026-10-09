param([string]$WindowsZip = '', [string]$AndroidApk = '')
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $repoRoot 'tools/third-party-notice-policy.ps1')
Add-Type -AssemblyName System.IO.Compression.FileSystem
$scratchRoot = Join-Path ([IO.Path]::GetTempPath()) ('Horde-ThirdPartyNotices-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $scratchRoot | Out-Null
$cases = 0
# Deliberately independent of the policy roster: dropping either scope or grant
# from the production policy must make these negative cases fail.
$noticeFixtures = @(
    @{ Source = 'LICENSE'; Entry = 'LICENSE' },
    @{ Source = 'LICENSE_SCOPE.md'; Entry = 'LICENSE_SCOPE.md' },
    @{ Source = 'third_party/cgltf/LICENSE'; Entry = 'THIRD_PARTY_NOTICES/cgltf-LICENSE.txt' }
)
function New-NoticeFixture([string]$Name, [string]$Platform, [string]$Target = '', [string]$Mutation = '') {
    $path = Join-Path $scratchRoot $Name
    $zip = [IO.Compression.ZipFile]::Open($path, [IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($notice in $noticeFixtures) {
            $entryName = $notice.Entry
            if ($Platform -eq 'Android') { $entryName = 'assets/' + $entryName }
            $bytes = [IO.File]::ReadAllBytes((Join-Path $repoRoot $notice.Source))
            $copies = 1
            if ($notice.Entry -ceq $Target) {
                switch ($Mutation) {
                    'missing' { $copies = 0 }
                    'truncated' { $bytes = [byte[]]$bytes[0..100] }
                    'corrupt' { $bytes[$bytes.Length - 1] = $bytes[$bytes.Length - 1] -bxor 1 }
                    'duplicate' { $copies = 2 }
                    'wrong-path' { $entryName = 'wrong/' + $entryName }
                    'wrong-case' { $entryName = $entryName.ToLowerInvariant() }
                    'scope-replaced-with-grant' { $bytes = [IO.File]::ReadAllBytes((Join-Path $repoRoot 'LICENSE')) }
                    default { throw "Unknown notice fixture mutation: $Mutation" }
                }
            }
            for ($i = 0; $i -lt $copies; ++$i) {
                $entry = $zip.CreateEntry($entryName)
                $stream = $entry.Open()
                try { $stream.Write($bytes, 0, $bytes.Length) } finally { $stream.Dispose() }
            }
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
    foreach ($notice in $noticeFixtures) {
        $sourceHash = (Get-FileHash -LiteralPath (Join-Path $repoRoot $notice.Source) -Algorithm SHA256).Hash
        $stagedHash = (Get-FileHash -LiteralPath (Join-Path $stage $notice.Entry) -Algorithm SHA256).Hash
        if ($sourceHash -cne $stagedHash) { throw "Staging changed complete notice bytes: $($notice.Entry)" }
    }
    $stagedZip = Join-Path $scratchRoot 'windows-staged.zip'
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $stagedZip
    Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $stagedZip -Platform Windows
    ++$cases
    foreach ($platform in @('Windows', 'Android')) {
        $valid = New-NoticeFixture "$platform-valid.zip" $platform
        Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repoRoot -ArchivePath $valid -Platform $platform
        ++$cases
        foreach ($notice in $noticeFixtures) {
            foreach ($mutation in @('missing', 'truncated', 'corrupt', 'duplicate', 'wrong-path', 'wrong-case')) {
                $name = "$platform-$($notice.Entry.Replace('/', '-'))-$mutation.zip"
                $invalid = New-NoticeFixture $name $platform $notice.Entry $mutation
                Assert-Rejected $invalid $platform
                ++$cases
            }
        }
        # The MIT grant alone cannot stand in for its asset/Core/mixed-bundle exclusions.
        $noScope = New-NoticeFixture "$platform-scope-replaced-with-grant.zip" $platform 'LICENSE_SCOPE.md' 'scope-replaced-with-grant'
        Assert-Rejected $noScope $platform
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
    if ($gradle -notmatch "(?s)from\('../..'\)\s*\{\s*include 'ASSET_LICENSES.md'\s*//[^\r\n]*\s*include 'LICENSE'\s*include 'LICENSE_SCOPE.md'\s*\}") {
        throw 'Android runtime-assets Sync must retain the asset summary, exact original-software grant and scope at their admitted APK paths.'
    }
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
