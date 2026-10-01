# Read-only, investigation-specific preflight. This does not inspect an APK,
# validate SPIR-V or replace the unchanged production containment gate.
function Assert-AndroidShowcaseCaptureSelection {
    param([string[]]$Selection, [Collections.IDictionary]$KnownZones)
    $seen = @{}
    foreach ($checkpoint in $Selection) {
        if (-not $KnownZones.Contains($checkpoint)) { throw "Unknown capture checkpoint '$checkpoint'." }
        if ($seen.ContainsKey($checkpoint)) { throw "Duplicate capture checkpoint '$checkpoint'." }
        $seen[$checkpoint] = $true
    }
}

function Resolve-AndroidShowcaseValidationTarget {
    param([bool]$StagedPrimary, [bool]$SkipBuild, [string]$ApkPath,
          [string]$ManifestPath, [string]$ViewmodelDirectory, [bool]$AnatomicalMount)
    if (-not $StagedPrimary) {
        if ($ManifestPath) { throw 'A staged manifest requires the explicit investigation selector.' }
        return [pscustomobject]@{ package = $(if ($ViewmodelDirectory) {
            'com.samfa12.hordelanternrt.debug.viewmodel'
        } else { 'com.samfa12.hordelanternrt.debug' }); expectedBundle = $null; manifestSha256 = $null }
    }
    if (-not $SkipBuild -or -not $ApkPath -or -not $ManifestPath -or $ViewmodelDirectory -or $AnatomicalMount) {
        throw 'Staged diagnostic validation requires SkipBuild, an immutable APK/manifest and unchanged accepted player assets.'
    }
    if (-not (Test-Path -LiteralPath $ApkPath -PathType Leaf)) { throw 'The immutable staged APK is missing.' }
    $manifestFile = Get-Item -LiteralPath $ManifestPath -ErrorAction Stop
    if ($manifestFile.PSIsContainer -or $manifestFile.Length -le 0 -or $manifestFile.Length -gt 131072) {
        throw 'A bounded staged manifest file is required.'
    }
    $manifest = Get-Content -Raw -LiteralPath $manifestFile.FullName | ConvertFrom-Json
    $roles = @($manifest.modules | ForEach-Object { "$($_.material):$($_.pass)" } | Sort-Object)
    if ($manifest.schema -ne 1 -or $manifest.classification -cne 'investigation-only-staged-primary' -or
        $manifest.instrumentation -cne 'Diagnostic' -or $manifest.quality -cne 'Mobile' -or
        $manifest.stage -cne 'rgen' -or @($manifest.modules).Count -ne 4 -or
        [string]::Join(',', $roles) -cne 'GenericDielectric:primary,GenericDielectric:shade,OpaqueFast:primary,OpaqueFast:shade' -or
        @($manifest.pairs).Count -ne 2) { throw 'Only the exact Diagnostic/Mobile staged four-pass manifest is admitted.' }
    $bundle = [ordered]@{}
    foreach ($strategy in @('opaqueFast', 'genericDielectric')) {
        $material = if ($strategy -ceq 'opaqueFast') { 'OpaqueFast' } else { 'GenericDielectric' }
        $stem = if ($strategy -ceq 'opaqueFast') { 'opaque' } else { 'generic' }
        $suffix = if ($strategy -ceq 'opaqueFast') { 'opaque_fast' } else { 'generic_dielectric' }
        $key = "staged_primary_v1_diagnostic_mobile_$suffix"
        $pairs = @($manifest.pairs | Where-Object { $_.key -ceq $key })
        if ($pairs.Count -ne 1 -or $pairs[0].sha256 -cnotmatch '^[0-9a-f]{64}$') { throw "Missing exact staged pair: $key" }
        $bytes = @()
        foreach ($pass in @('primary', 'shade')) {
            $row = @($manifest.modules | Where-Object { $_.material -ceq $material -and $_.pass -ceq $pass })[0]
            if ($row.spirv -cne "$stem-$pass.rgen.spv" -or $row.bytes -le 20 -or $row.bytes -gt 2097152 -or
                $row.spirvSha256 -cnotmatch '^[0-9a-f]{64}$') { throw 'Staged module metadata/path is invalid.' }
            $module = Get-Item -LiteralPath (Join-Path $manifestFile.DirectoryName $row.spirv) -ErrorAction Stop
            if ($module.Length -ne $row.bytes -or ($module.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
                (Get-FileHash -LiteralPath $module.FullName -Algorithm SHA256).Hash.ToLowerInvariant() -cne $row.spirvSha256) {
                throw "Staged module file differs from the retained manifest: $($row.spirv)"
            }
            $bytes += ,([IO.File]::ReadAllBytes($module.FullName))
        }
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $pairHash = ([BitConverter]::ToString($sha.ComputeHash([byte[]]($bytes[0] + $bytes[1])))).Replace('-', '').ToLowerInvariant() }
        finally { $sha.Dispose() }
        if ($pairHash -cne $pairs[0].sha256) { throw "Staged pair digest mismatch: $key" }
        $bundle[$strategy] = [pscustomobject]@{ key=$key; sha256=$pairHash }
    }
    return [pscustomobject]@{ package='com.samfa12.hordelanternrt.debug.staged'; expectedBundle=[pscustomobject]$bundle;
        manifestSha256=(Get-FileHash -LiteralPath $manifestFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
}

function Assert-AndroidStagedPrimaryBundle {
    param($Bundle, $ExpectedBundle)
    foreach ($strategy in @('opaqueFast', 'genericDielectric')) {
        $entry = $Bundle.$strategy
        if ($null -ne $ExpectedBundle) {
            if ($entry.key -cne $ExpectedBundle.$strategy.key -or $entry.sha256 -cne $ExpectedBundle.$strategy.sha256) {
                throw 'Runtime selected staged pair does not match the exact admitted Diagnostic/Mobile artifacts.'
            }
        } elseif ([string]$entry.key -clike 'staged_primary_*') {
            throw 'Staged runtime evidence requires explicit investigation admission.'
        }
    }
}
