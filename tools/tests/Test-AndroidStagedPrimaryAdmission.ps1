$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../AndroidStagedPrimaryAdmission.ps1')
$passed = 0
function Assert([bool]$Ok, [string]$Why) { if (-not $Ok) { throw $Why }; ++$script:passed }
function Reject([scriptblock]$Work, [string]$Why) { $rejected=$false; try { & $Work | Out-Null } catch { $rejected=$true }; Assert $rejected $Why }
function Sha([byte[]]$Bytes) {
    $hash=[Security.Cryptography.SHA256]::Create()
    try { ([BitConverter]::ToString($hash.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant() }
    finally { $hash.Dispose() }
}
Assert ((Resolve-AndroidShowcaseValidationTarget).package -ceq 'com.samfa12.hordelanternrt.debug') 'Normal Debug target changed'
Assert ((Resolve-AndroidShowcaseValidationTarget -ViewmodelDirectory 'existing').package -ceq 'com.samfa12.hordelanternrt.debug.viewmodel') 'Existing viewmodel target changed'
Reject { Resolve-AndroidShowcaseValidationTarget -ManifestPath 'unrequested' } 'Implicit investigation accepted'
Reject { Resolve-AndroidShowcaseValidationTarget -StagedPrimary $true } 'Missing immutable inputs accepted'
$tempParent=[IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$tempRoot=Join-Path $tempParent ('horde-staged-admission-'+[guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $tempRoot
try {
    $apk=Join-Path $tempRoot 'fixture.apk'; [IO.File]::WriteAllBytes($apk, [byte[]]@(1,2,3))
    $manifestPath=Join-Path $tempRoot 'staged-primary-manifest.json'
    $modules=@(); $pairs=@()
    foreach ($material in @('OpaqueFast','GenericDielectric')) {
        $stem=if($material -ceq 'OpaqueFast'){'opaque'}else{'generic'}
        $suffix=if($material -ceq 'OpaqueFast'){'opaque_fast'}else{'generic_dielectric'}
        $payloads=@()
        foreach($pass in @('primary','shade')) {
            # Byte-level provenance fixture only, deliberately not real SPIR-V.
            $bytes=[byte[]](1..32 | ForEach-Object { $_ + $(if($pass -ceq 'shade'){1}else{0}) })
            $name="$stem-$pass.rgen.spv"; [IO.File]::WriteAllBytes((Join-Path $tempRoot $name),$bytes)
            $modules += [ordered]@{material=$material;pass=$pass;spirv=$name;bytes=$bytes.Length;spirvSha256=(Sha $bytes)}
            $payloads += ,$bytes
        }
        $pairs += [ordered]@{key="staged_primary_v1_diagnostic_mobile_$suffix";sha256=(Sha ([byte[]]($payloads[0]+$payloads[1])))}
    }
    $manifest=[ordered]@{schema=1;classification='investigation-only-staged-primary';instrumentation='Diagnostic';quality='Mobile';stage='rgen';modules=$modules;pairs=$pairs}
    $canonicalManifest=$manifest | ConvertTo-Json -Depth 8
    function SaveFixture { $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath }
    function Target { Resolve-AndroidShowcaseValidationTarget -StagedPrimary $true -SkipBuild $true -ApkPath $apk -ManifestPath $manifestPath }
    SaveFixture
    $target=Target
    Assert ($target.package -ceq 'com.samfa12.hordelanternrt.debug.staged' -and $target.manifestSha256.Length -eq 64) 'Wrong isolated target'
    Assert-AndroidStagedPrimaryBundle -Bundle $target.expectedBundle -ExpectedBundle $target.expectedBundle
    ++$passed
    Reject { Assert-AndroidStagedPrimaryBundle -Bundle $target.expectedBundle } 'Staged bundle accepted without selector'
    $wrong=$target.expectedBundle | ConvertTo-Json -Depth 4 | ConvertFrom-Json
    $wrong.opaqueFast.sha256='0'*64
    Reject { Assert-AndroidStagedPrimaryBundle -Bundle $wrong -ExpectedBundle $target.expectedBundle } 'Wrong runtime hash accepted'
    $wrong=$target.expectedBundle | ConvertTo-Json -Depth 4 | ConvertFrom-Json
    $wrong.opaqueFast.key='rayquery_compute_diagnostic_mobile_opaque_fast'
    Reject { Assert-AndroidStagedPrimaryBundle -Bundle $wrong -ExpectedBundle $target.expectedBundle } 'Wrong runtime backend accepted'
    Reject { Resolve-AndroidShowcaseValidationTarget -StagedPrimary $true -SkipBuild $false -ApkPath $apk -ManifestPath $manifestPath } 'Silent rebuild accepted'
    Reject { Resolve-AndroidShowcaseValidationTarget -StagedPrimary $true -SkipBuild $true -ApkPath $apk -ManifestPath $manifestPath -ViewmodelDirectory 'overlay' } 'Asset overlay accepted'
    Reject { Resolve-AndroidShowcaseValidationTarget -StagedPrimary $true -SkipBuild $true -ApkPath $apk -ManifestPath $manifestPath -AnatomicalMount $true } 'Mount change accepted'
    Reject { Resolve-AndroidShowcaseValidationTarget -StagedPrimary $true -SkipBuild $true -ApkPath "$apk-missing" -ManifestPath $manifestPath } 'Missing APK accepted'
    foreach($change in @('Shipping','High','duplicate','path','pairHash','moduleHash')) {
        switch($change) {
            Shipping { $manifest.instrumentation='Shipping' }
            High { $manifest.quality='High' }
            duplicate { $manifest.modules[1].pass='primary' }
            path { $manifest.modules[0].spirv='../escape.spv' }
            pairHash { $manifest.pairs[0].sha256='0'*64 }
            moduleHash { $manifest.modules[0].spirvSha256='0'*64 }
        }
        SaveFixture; Reject { Target } "Invalid $change fixture admitted"
        # Each negative must start from, and restore, an independently valid
        # manifest. Avoid accidentally passing later negatives on stale damage.
        $manifest=$canonicalManifest | ConvertFrom-Json
        SaveFixture
        Assert ((Target).package -ceq $target.package) "Valid fixture not restored after $change"
    }
    [IO.File]::WriteAllBytes((Join-Path $tempRoot 'opaque-primary.rgen.spv'), [byte[]](2..33))
    Reject { Target } 'Tampered on-disk module accepted'
    Write-Output "PASS $passed bounded staged Android admission assertions; fake bytes, no Vulkan/ADB evidence."
} finally {
    $resolved=[IO.Path]::GetFullPath($tempRoot)
    if (-not $resolved.StartsWith($tempParent,[StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolved) -notmatch '^horde-staged-admission-[0-9a-f]{32}$') { throw 'Unsafe fixture cleanup target' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
