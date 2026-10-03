param(
    [Parameter(Mandatory=$true)][string]$Destination,
    [switch]$ValidateOnly,
    [switch]$FinalizeManifest
)
$ErrorActionPreference='Stop'
if($ValidateOnly -and -not $FinalizeManifest){$mode='preflight'}
elseif($FinalizeManifest){$mode=if($ValidateOnly){'manifest-preflight'}else{'manifest'}}
else{$mode='curate'}

$sourceRoot=[IO.Path]::GetFullPath($PSScriptRoot)
$expectedSourceRoot='C:\Dev\tmp\horde-first-blocker-20260930'
if(-not $sourceRoot.Equals([IO.Path]::GetFullPath($expectedSourceRoot),[StringComparison]::OrdinalIgnoreCase)){
    throw "Run only from the assigned external scratch root: $expectedSourceRoot"
}
$target=[IO.Path]::GetFullPath($Destination)
$engineeringRoot='C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.6.1-engineering-pass'
$evidenceRoot=Join-Path $engineeringRoot 'docs\evidence'
$expectedTarget=Join-Path $evidenceRoot '2026-09-30-first-blocker'
if(-not $target.Equals([IO.Path]::GetFullPath($expectedTarget),[StringComparison]::OrdinalIgnoreCase)){
    throw "Target must be the exact new evidence directory: $expectedTarget"
}
function AssertNoReparseInTargetPath([string]$Path){
    $cursor=[IO.Path]::GetFullPath($Path)
    while(-not [string]::IsNullOrWhiteSpace($cursor)){
        if(Test-Path -LiteralPath $cursor){
            $entry=Get-Item -LiteralPath $cursor -Force
            if(($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0){
                throw "Reparse point forbidden in evidence path: $cursor"
            }
        }
        $parent=[IO.Path]::GetDirectoryName($cursor)
        if([string]::IsNullOrWhiteSpace($parent) -or $parent -eq $cursor){break}
        $cursor=$parent
    }
}
function AssertNoReparseInTargetTree([string]$Path){
    if(-not (Test-Path -LiteralPath $Path -PathType Container)){return}
    foreach($entry in @(Get-ChildItem -LiteralPath $Path -Force -Recurse)){
        if(($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0){
            throw "Reparse point forbidden inside evidence target: $($entry.FullName)"
        }
    }
}
function AssertReadmeOnlyTarget([string]$Path){
    AssertNoReparseInTargetPath $Path
    if(-not (Test-Path -LiteralPath $Path)){return}
    $rootEntry=Get-Item -LiteralPath $Path -Force
    if(-not $rootEntry.PSIsContainer){throw 'Existing evidence target is not a directory'}
    $entries=@(Get-ChildItem -LiteralPath $Path -Force -Recurse)
    AssertNoReparseInTargetTree $Path
    foreach($entry in $entries){
        if($entry.PSIsContainer){throw "Existing evidence target has an unexpected subdirectory: $($entry.FullName)"}
    }
    if($entries.Count -ne 1 -or $entries[0].Name -cne 'README.md'){
        throw 'Existing target is allowed only when its sole file is the main-authored README.md'
    }
}
AssertNoReparseInTargetPath $target
$targetExists=Test-Path -LiteralPath $target
if($mode -eq 'preflight' -or $mode -eq 'curate'){
    AssertReadmeOnlyTarget $target
}

$plan=[Collections.Generic.List[object]]::new()
$manifest=[Collections.Generic.List[object]]::new()
$runPlans=[Collections.Generic.List[object]]::new()
function AddEvidence([string]$Source,[string]$Output=$null,[switch]$Compress,[switch]$Optional){
    if([string]::IsNullOrWhiteSpace($Output)){$Output=$Source}
    $sourcePath=[IO.Path]::GetFullPath((Join-Path $sourceRoot $Source))
    $sourcePrefix=$sourceRoot.TrimEnd('\')+'\'
    if(-not $sourcePath.StartsWith($sourcePrefix,[StringComparison]::OrdinalIgnoreCase)){
        throw "Source escaped bounded scratch root: $Source"
    }
    $extension=[IO.Path]::GetExtension($sourcePath).ToLowerInvariant()
    if($extension -notin @('.json','.jsonl','.txt','.log','.md','.ps1','.py','.patch','.glsl')){
        throw "Disallowed non-text evidence type: $Source"
    }
    if($Source -match '(^|[\\/])[^\\/]*(\.apk|\.so|\.spv|\.spirv)$'){
        throw "Application or shader binary forbidden by allowlist: $Source"
    }
    $outputPath=$Output.Replace('\','/')
    if($outputPath.StartsWith('/') -or $outputPath -match '(^|/)\.\.?(/|$)'){
        throw "Unsafe curated relative path: $Output"
    }
    if($Compress){$outputPath+='.gz'}
    $plan.Add([ordered]@{source=$Source.Replace('\','/');output=$outputPath;compressed=[bool]$Compress;optional=[bool]$Optional})
}

$runIds=@(
    'first-blocker-control-a1-route-20260930',
    'first-blocker-candidate-b1-route-20260930',
    'first-blocker-candidate-b2-route-20260930',
    'first-blocker-control-a2-route-20260930'
)
foreach($runId in $runIds){
    $runRoot="phone/$runId"
    $required=@()
    foreach($file in @('trial.json','analysis.json','context-samples.jsonl','launch.log','pull.log')){
        $required+="$runRoot/$file"
        AddEvidence "$runRoot/$file"
    }
    $required+="$runRoot/$runId/result.json"
    $required+="$runRoot/$runId/benchmark.json"
    AddEvidence "$runRoot/$runId/result.json"
    AddEvidence "$runRoot/$runId/benchmark.json" -Compress
    $runPlans.Add([ordered]@{runId=$runId;files=$required})
}

$installFiles=@('receipt.json','install.log','pull.log')
foreach($install in @('first-blocker-a1-control','first-blocker-b1-candidate','first-blocker-a2-control')){
    foreach($file in $installFiles){AddEvidence "installs/$install/$file"}
}

foreach($file in @('artifact.json','containment.json')){AddEvidence "artifacts/control/$file"}
foreach($file in @('raygen-variant-catalog.json','rayquery-variant-catalog.json')){
    AddEvidence "artifacts/benchmark/control-reference-source/tools/$file" "artifacts/control/$file"
}
foreach($file in @('artifact.json','containment.json','asset-shader-comparison.json','raygen-variant-catalog.json','rayquery-variant-catalog.json')){
    AddEvidence "artifacts/benchmark/$file"
}
foreach($file in @('provenance.json','provenance-notes.md','two-hand-edited-files.patch')){AddEvidence "artifacts/$file"}

AddEvidence 'ray-flag-audit/audit-ray-query-flags.ps1'
AddEvidence 'ray-flag-audit/results-sealed/ray-query-flag-audit.json'
if(Test-Path -LiteralPath (Join-Path $sourceRoot 'ray-flag-audit/rejected-audit.log') -PathType Leaf){
    AddEvidence 'ray-flag-audit/rejected-audit.log'
}

foreach($file in @(
    'PLAN.md','run-trial.ps1','install-trial-artifact.ps1','analyse-trial.ps1',
    'parser-tests.json','compare-trials.ps1','comparison.json','DECISION.md',
    'curate-first-blocker-evidence.ps1',
    'ci-logs/36700295969.log.txt','ci-logs/36700301099.log.txt','ci-logs/ci-receipt.json',
    'android-build.log','compatibility-generic-build.log','compatibility-legacy-build.log',
    'compute-build.log','msvc-debug-build.log','msvc-debug-test.log',
    'msvc-release-build.log','msvc-release-test.log','pipeline-adapter.log',
    'raygen-build.log','shader-artifact-test-2.log','shader-artifact-test-3.log',
    'shader-contract-tests.log','windows-debug-build-2.log','windows-debug-build.log',
    'windows-standard-compute.stderr.log','windows-standard-compute.stdout.log',
    'windows-standard-pipeline.stderr.log','windows-standard-pipeline.stdout.log',
    'windows-pipeline-image-comparison.json',
    'windows-standard-compute/capture-manifest.json',
    'windows-standard-pipeline/capture-manifest.json',
    'phone-image-comparison.json',
    'verify-debug-restoration.ps1','debug-restoration-comparison.json',
    'phone-images-restored.log','phone-restoration-timeout-native.log',
    'phone-images-restored-retry.log'
)){AddEvidence $file}

foreach($file in @(
    'restore-receipt.json','cmake-wrapper-repair.json','restored-hand-source-check.log',
    'character-slot-debug-build.log','focused-shader-and-character-debug-ctest.log',
    'final-compute-shader-contract.log','character-slot-debug-build-restored.log',
    'character-slot-release-build-restored.log','character-slot-release-ctest.log',
    'final-compat-generic-check.log','final-compat-legacy-check.log',
    'final-pipeline-adapter-check.log','final-compute-check.log',
    'final-pipeline-check-catalog-corrected.log','compute-wrapper-reconfigure.log',
    'rayquery-compute-ctest-wrapper-rerun.log'
)){AddEvidence "restore-control/$file"}

foreach($file in @(
    'capture-manifest.json','summary.json','vulkan_capability_report.json',
    'route-replay-state.json','capture-01-lantern-glass-production-state.json'
)){
    AddEvidence "phone-images-restored-retry/run-20260930-204834/$file" "phone-images/restored/run-20260930-204834/$file"
}

foreach($pair in @(
    @{label='control';run='run-20260930-194952';states=@('capture-01-opening-state.json','capture-02-worst-bend-state.json')},
    @{label='candidate';run='run-20260930-195349';states=@(
        'capture-01-opening-state.json','capture-02-worst-bend-state.json',
        'capture-03-lantern-glass-production-state.json','capture-04-player-viewmodel-lantern-high-look-up-state.json',
        'capture-05-player-viewmodel-lantern-low-parry-state.json','capture-06-glass-edge-fresnel-state.json',
        'capture-07-glass-millimetre-closed-state.json','capture-08-glass-tinted-transport-state.json',
        'capture-09-glass-fire-transport-state.json'
    )}
)){
    $imageRoot="phone-images-$($pair.label)/$($pair.run)"
    foreach($file in @('capture-manifest.json','summary.json','vulkan_capability_report.json','route-replay-state.json') + $pair.states){
        AddEvidence "$imageRoot/$file" "phone-images/$($pair.label)/$($pair.run)/$file"
    }
}

$destinations=@($plan | ForEach-Object {$_.output})
if(@($destinations | Sort-Object -Unique).Count -ne $destinations.Count){
    throw 'Duplicate curated output path in explicit allowlist'
}

$sourceMissing=[Collections.Generic.List[string]]::new()
foreach($item in $plan){
    if(-not (Test-Path -LiteralPath (Join-Path $sourceRoot $item.source) -PathType Leaf)){
        if(-not $item.optional){$sourceMissing.Add($item.source)}
    }
}
if($mode -eq 'preflight'){
    Write-Output "ALLOWLIST PLAN $($plan.Count) entries across $($runPlans.Count) required runs; no files written."
    foreach($runPlan in $runPlans){
        $runMissing=@($runPlan.files | Where-Object {-not (Test-Path -LiteralPath (Join-Path $sourceRoot $_) -PathType Leaf)})
        $phase=if($runPlan.runId -eq 'first-blocker-control-a2-route-20260930'){' required second control'}else{''}
        if($runMissing.Count){Write-Output "RUN PENDING $($runPlan.runId):$phase missing $($runMissing -join ', ')"}
        else{Write-Output "RUN READY $($runPlan.runId):$phase"}
    }
    if($targetExists){AssertReadmeOnlyTarget $target}
    if($sourceMissing.Count){throw "Preflight incomplete; exact allowlist evidence missing: $($sourceMissing -join ', ')"}
    Write-Output "Exact lighting evidence allowlist preflight PASS ($($plan.Count) entries); no files written."
    return
}
if($sourceMissing.Count){throw "Exact allowlist evidence missing: $($sourceMissing -join ', ')"}

if($mode -eq 'curate'){
    if($targetExists){AssertReadmeOnlyTarget $target}
    else{New-Item -ItemType Directory -Path $target | Out-Null}
    foreach($item in $plan){
        $sourcePath=Join-Path $sourceRoot $item.source
        $outputPath=Join-Path $target $item.output
        $parent=Split-Path -Parent $outputPath
        if(-not (Test-Path -LiteralPath $parent -PathType Container)){New-Item -ItemType Directory -Path $parent -Force | Out-Null}
        if(Test-Path -LiteralPath $outputPath){throw "Refuse to overwrite curated file: $($item.output)"}
        if($item.compressed){
            $sourceHash=(Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
            $sourceBytes=(Get-Item -LiteralPath $sourcePath).Length
            $inputStream=[IO.File]::OpenRead($sourcePath)
            $outputStream=[IO.File]::Open($outputPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
            $encoder=[IO.Compression.GZipStream]::new($outputStream,[IO.Compression.CompressionLevel]::Optimal)
            try{$inputStream.CopyTo($encoder)}finally{$encoder.Dispose();$outputStream.Dispose();$inputStream.Dispose()}
            $compressed=[IO.File]::OpenRead($outputPath)
            $decoder=[IO.Compression.GZipStream]::new($compressed,[IO.Compression.CompressionMode]::Decompress)
            $incremental=[Security.Cryptography.IncrementalHash]::CreateHash([Security.Cryptography.HashAlgorithmName]::SHA256)
            $buffer=New-Object byte[] 65536
            $roundtripBytes=0L
            try{
                while(($read=$decoder.Read($buffer,0,$buffer.Length)) -gt 0){
                    $incremental.AppendData($buffer,0,$read)
                    $roundtripBytes+=$read
                }
                $roundtripHash=[Convert]::ToHexString($incremental.GetHashAndReset()).ToLowerInvariant()
            }finally{$incremental.Dispose();$decoder.Dispose();$compressed.Dispose()}
            if($roundtripHash -cne $sourceHash -or $roundtripBytes -ne $sourceBytes){
                throw "Gzip roundtrip differs from source: $($item.source)"
            }
        }else{
            [IO.File]::Copy($sourcePath,$outputPath,$false)
        }
    }
    Write-Output "Curated exact allowlist ($($plan.Count) entries). No APK/ELF/SPIR-V or manifest written; add README.md, then use -FinalizeManifest."
    return
}

if($mode -eq 'manifest' -or $mode -eq 'manifest-preflight'){
    if(-not (Test-Path -LiteralPath $target -PathType Container)){throw 'Manifest operation requires the already-curated exact target'}
    AssertNoReparseInTargetPath $target
    AssertNoReparseInTargetTree $target
    $readme=Join-Path $target 'README.md'
    if(-not (Test-Path -LiteralPath $readme -PathType Leaf)){throw 'Main must add README.md before manifest generation'}
    $manifestPath=Join-Path $target 'SHA256SUMS.json'
    if(Test-Path -LiteralPath $manifestPath){throw 'Refuse to overwrite an existing evidence manifest'}
    $expectedTargetFiles=@($destinations + 'README.md' | Sort-Object -Unique)
    $actualTargetFiles=@(Get-ChildItem -LiteralPath $target -Force -Recurse -File | ForEach-Object {
        [IO.Path]::GetRelativePath($target,$_.FullName).Replace('\','/')
    } | Sort-Object -Unique)
    $expectedDirectories=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach($output in $destinations){
        $parent=[IO.Path]::GetDirectoryName($output.Replace('/','\'))
        while(-not [string]::IsNullOrWhiteSpace($parent)){
            [void]$expectedDirectories.Add($parent.Replace('\','/'))
            $parent=[IO.Path]::GetDirectoryName($parent)
        }
    }
    $actualDirectories=@(Get-ChildItem -LiteralPath $target -Force -Recurse -Directory | ForEach-Object {
        [IO.Path]::GetRelativePath($target,$_.FullName).Replace('\','/')
    })
    $unexpectedDirectories=@($actualDirectories | Where-Object {-not $expectedDirectories.Contains($_)})
    if($unexpectedDirectories.Count){throw "Unexpected directory(s) in evidence target: $($unexpectedDirectories -join ', ')"}
    $expectedSet=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $actualSet=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach($path in $expectedTargetFiles){[void]$expectedSet.Add($path)}
    foreach($path in $actualTargetFiles){[void]$actualSet.Add($path)}
    $unexpected=@($actualTargetFiles | Where-Object {-not $expectedSet.Contains($_)})
    $missing=@($expectedTargetFiles | Where-Object {-not $actualSet.Contains($_)})
    if($unexpected.Count -gt 0){throw "Unexpected file(s) in evidence target: $($unexpected -join ', ')"}
    if($missing.Count -gt 0){throw "Curated file(s) missing before manifest: $($missing -join ', ')"}
    foreach($item in $plan){
        $sourcePath=Join-Path $sourceRoot $item.source
        $sourceHash=(Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
        $sourceBytes=(Get-Item -LiteralPath $sourcePath).Length
        $outputPath=Join-Path $target $item.output
        if(-not (Test-Path -LiteralPath $outputPath -PathType Leaf)){throw "Curated evidence missing: $($item.output)"}
        $outputHash=(Get-FileHash -LiteralPath $outputPath -Algorithm SHA256).Hash.ToLowerInvariant()
        $outputBytes=(Get-Item -LiteralPath $outputPath).Length
        $roundtripHash=$sourceHash
        $roundtripBytes=$sourceBytes
        if($item.compressed){
            $compressed=[IO.File]::OpenRead($outputPath)
            $decoder=[IO.Compression.GZipStream]::new($compressed,[IO.Compression.CompressionMode]::Decompress)
            $incremental=[Security.Cryptography.IncrementalHash]::CreateHash([Security.Cryptography.HashAlgorithmName]::SHA256)
            $buffer=New-Object byte[] 65536
            $roundtripBytes=0L
            try{
                while(($read=$decoder.Read($buffer,0,$buffer.Length)) -gt 0){
                    $incremental.AppendData($buffer,0,$read)
                    $roundtripBytes+=$read
                }
                $roundtripHash=[Convert]::ToHexString($incremental.GetHashAndReset()).ToLowerInvariant()
            }finally{$incremental.Dispose();$decoder.Dispose();$compressed.Dispose()}
            if($roundtripHash -cne $sourceHash -or $roundtripBytes -ne $sourceBytes){
                throw "Gzip roundtrip differs from source: $($item.source)"
            }
        }elseif($outputHash -cne $sourceHash -or $outputBytes -ne $sourceBytes){
            throw "Curated bytes differ from source: $($item.source)"
        }
        $manifest.Add([ordered]@{
            path=$item.output;sha256=$outputHash;bytes=$outputBytes
            sourceSha256=$sourceHash;sourceBytes=$sourceBytes
            compressed=$item.compressed;roundtripSha256=$roundtripHash;roundtripBytes=$roundtripBytes
        })
    }
    if($mode -eq 'manifest-preflight'){
        Write-Output "Manifest preflight PASS ($($plan.Count + 1) allowed files including README.md); no files written."
        return
    }
    $readmePath=Join-Path $target 'README.md'
    $manifest.Add([ordered]@{
        path='README.md';sha256=(Get-FileHash -LiteralPath $readmePath -Algorithm SHA256).Hash.ToLowerInvariant()
        bytes=(Get-Item -LiteralPath $readmePath).Length;sourceSha256=$null;sourceBytes=$null
        compressed=$false;roundtripSha256=$null;roundtripBytes=$null
    })
    $json=$manifest | ConvertTo-Json -Depth 6
    $stream=[IO.File]::Open((Join-Path $target 'SHA256SUMS.json'),[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try{
        $bytes=[Text.UTF8Encoding]::new($false).GetBytes($json)
        $stream.Write($bytes,0,$bytes.Length)
    }finally{$stream.Dispose()}
    Write-Output "Generated additive SHA256SUMS.json for $($manifest.Count) exact allowlisted files after README.md was present."
}
