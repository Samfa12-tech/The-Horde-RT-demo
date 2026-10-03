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
$expectedSourceRoot='C:\Dev\tmp\horde-generic-route-profile-20260930'
if(-not $sourceRoot.Equals([IO.Path]::GetFullPath($expectedSourceRoot),[StringComparison]::OrdinalIgnoreCase)){
    throw "Run only from the assigned external scratch root: $expectedSourceRoot"
}
$target=[IO.Path]::GetFullPath($Destination)
$engineeringRoot='C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.6.1-engineering-pass'
$evidenceRoot=Join-Path $engineeringRoot 'docs\evidence'
$expectedTarget=Join-Path $evidenceRoot '2026-09-30-generic-strategy'
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
$profileReceiptPath=Join-Path $sourceRoot 'profile/provenance/profile-build-receipt.json'
if(Test-Path -LiteralPath $profileReceiptPath -PathType Leaf){
    $profileReceipt=Get-Content -LiteralPath $profileReceiptPath -Raw | ConvertFrom-Json
    if($profileReceipt.source.head -cne 'eafbf8262442a82e0835edf5cf5306d718e64633' -or
       $profileReceipt.source.patchSha256 -cne '0775f22d28e91966707db8e62407eb5738c1654e97da30fa239792f8dbd4c474'){
        throw 'Refuse evidence whose profile source/forced-strategy patch differs from the frozen eaf receipt.'
    }
} else {
    $profileReceipt=$null
}
function AddEvidence([string]$Source,[string]$Output=$null,[switch]$Compress,[switch]$Optional){
    if([string]::IsNullOrWhiteSpace($Output)){$Output=$Source}
    $sourcePath=[IO.Path]::GetFullPath((Join-Path $sourceRoot $Source))
    $sourcePrefix=$sourceRoot.TrimEnd('\')+'\'
    if(-not $sourcePath.StartsWith($sourcePrefix,[StringComparison]::OrdinalIgnoreCase)){
        throw "Source escaped bounded scratch root: $Source"
    }
    $extension=[IO.Path]::GetExtension($sourcePath).ToLowerInvariant()
    if($extension -notin @('.json','.jsonl','.txt','.log','.md','.ps1','.py','.patch','.glsl','.png')){
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

$warmupId='generic-route-control-warmup-20260930'
$warmupRoot="phone/$warmupId"
$warmupFiles=@(
    "$warmupRoot/trial.json",
    "$warmupRoot/context-samples.jsonl",
    "$warmupRoot/$warmupId/result.json",
    "$warmupRoot/$warmupId/benchmark.json"
)
$runPlans.Add([ordered]@{runId=$warmupId;classification='control warmup, context only, excluded from ABBA';files=$warmupFiles})
AddEvidence "$warmupRoot/trial.json" 'trials/control-warmup/trial.json'
AddEvidence "$warmupRoot/context-samples.jsonl" 'trials/control-warmup/context-samples.jsonl'
AddEvidence "$warmupRoot/$warmupId/result.json" 'trials/control-warmup/result.json'
AddEvidence "$warmupRoot/$warmupId/benchmark.json" 'trials/control-warmup/benchmark.json' -Compress

# Frozen build provenance. No APK, ELF, SPIR-V, or signing file is admitted.
AddEvidence 'profile/provenance/profile-build-receipt.json'
AddEvidence 'profile/provenance/containment-control-benchmark-shipping-mobile-eaf-scanner.json' 'provenance/containment-control-shipping-mobile.json'
AddEvidence 'profile/provenance/containment-generic-benchmark-shipping-mobile.json' 'provenance/containment-generic-shipping-mobile.json'
AddEvidence 'profile/provenance/containment-generic-debug-diagnostic-mobile.json' 'provenance/containment-generic-debug-diagnostic-mobile.json'
AddEvidence 'profile/provenance/asset-shader-comparison-eaf-scanner.json' 'provenance/asset-shader-comparison.json'
AddEvidence 'profile/provenance/raygen-variant-catalog.json' 'provenance/raygen-variant-catalog.json'
AddEvidence 'profile/provenance/rayquery-variant-catalog.json' 'provenance/rayquery-variant-catalog.json'
AddEvidence 'profile/provenance/lead-byte-verification.json' 'provenance/lead-byte-verification.json'
AddEvidence 'profile/provenance/strategy-seam-review.md' 'provenance/strategy-seam-review.md'
AddEvidence 'force-generic-investigation.patch' 'probe/force-generic-investigation.patch'

# Exact method and review artifacts, preserving both failed and passing checks.
foreach($file in @('run-trial.ps1','install-trial-artifact.ps1','analyse-trial.ps1','compare-trials.ps1','compare-opening.ps1','verify-build-lead.ps1','curate-generic-strategy-evidence.ps1')){
    AddEvidence $file "method/$file"
}
foreach($file in @(
    'profile/logs/gradle-assembleBenchmark-assembleDebug-arm64-20260930.log',
    'opening-image-verifier-green.log',
    'opening-image-comparison.json',
    'profile/provenance/negative-parser-tests.json',
    'profile/logs/negative-generic-route-negative-wrong-apk-20260930.log',
    'profile/logs/negative-generic-route-negative-wrong-strategy-20260930.log',
    'profile/logs/negative-generic-route-negative-missing-identity-20260930.log'
)){AddEvidence $file}

# Comparator regression fixtures are synthetic metadata only; never curate the
# fake analysis.json inputs as phone runs or place them in the real phone tree.
$fixtureRoot='profile/comparison-fixtures/property-count-fix-20260930'
foreach($file in @(
    'fixture-only.json',
    'positive-validate-only.log',
    'negative-validate-only-final.log',
    'property-count-fix-test-receipt.json',
    'syntax-and-scope-check.json'
)){AddEvidence "$fixtureRoot/$file" "tests/comparator/$file"}

# Small authored-scene evidence only; omit package/device dumps, timing dumps,
# personal media, and repeated identical PNGs.
$controlImage='images/control/run-20260930-223817'
foreach($file in @('capture-manifest.json','summary.json','capture-01-opening-state.json','capture-01-opening-75.png','route-replay-state.json')){
    AddEvidence "$controlImage/$file" "captures/control/$file"
}
$profileImage='images/generic-profile-focused/run-20260930-225102'
foreach($file in @('capture-manifest.json','summary.json','capture-01-opening-state.json','capture-01-opening-75.png','vulkan_capability_report.json')){
    AddEvidence "$profileImage/$file" "captures/generic-profile-focused/$file"
}
$timeoutImage='images/generic-profile/run-20260930-224329'
AddEvidence "$timeoutImage/timeout-native-state.json" 'captures/generic-profile-timeout/timeout-native-state.json'
AddEvidence "$timeoutImage/timeout-app-logcat.txt" 'captures/generic-profile-timeout/timeout-app-logcat.txt'

# Validate all retained timeout logcat lines are for the one observed app PID.
$timeoutLogPath=Join-Path $sourceRoot "$timeoutImage/timeout-app-logcat.txt"
if(Test-Path -LiteralPath $timeoutLogPath -PathType Leaf){
    $observedPids=@(Get-Content -LiteralPath $timeoutLogPath | ForEach-Object {
        if($_ -match '^\S+\s+\S+\s+(\d+)\s+\d+\s'){$Matches[1]}
    } | Sort-Object -Unique)
    if($observedPids.Count -ne 1 -or $observedPids[0] -cne '17098'){
        throw "Timeout log must contain only observed app PID 17098; found: $($observedPids -join ', ')"
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
    foreach($runPlan in $runPlans){
        $runMissing=@($runPlan.files | Where-Object {-not (Test-Path -LiteralPath (Join-Path $sourceRoot $_) -PathType Leaf)})
        if($runMissing.Count){Write-Output "RUN PENDING $($runPlan.runId): $($runMissing -join ', ')"}
        else{Write-Output "RUN READY $($runPlan.runId) ($($runPlan.classification))"}
    }
    if($targetExists){AssertReadmeOnlyTarget $target}
    if($sourceMissing.Count){throw "Preflight incomplete; exact allowlist evidence missing: $($sourceMissing -join ', ')"}
    Write-Output "Exact Generic strategy evidence allowlist preflight PASS ($($plan.Count) entries); no files written."
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
    Write-Output "Curated Generic strategy evidence allowlist ($($plan.Count) entries). No APK/ELF/SPIR-V included; additive manifest remains a separate -FinalizeManifest step."
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
    $controlCapture=Get-Content -LiteralPath (Join-Path $sourceRoot 'images/control/run-20260930-223817/capture-manifest.json') -Raw | ConvertFrom-Json
    $profileCapture=Get-Content -LiteralPath (Join-Path $sourceRoot 'images/generic-profile-focused/run-20260930-225102/capture-manifest.json') -Raw | ConvertFrom-Json
    if($controlCapture.sourceDirty -ne $true -or $profileCapture.sourceDirty -ne $true){
        throw 'Expected dirty capture-harness metadata; README and manifest must keep it distinct from exact APK provenance.'
    }
    $manifestDocument=[ordered]@{
        schema=1
        evidenceClass='investigation-only-whole-strategy; incomplete matched ABBA checkpoint'
        profileSource=[ordered]@{
            commit=$profileReceipt.source.head
            detachedWorktree=$profileReceipt.source.worktree
            dirtyTrackedPaths=$profileReceipt.source.dirtyTrackedPaths
            forcedStrategyPatchSha256=$profileReceipt.source.patchSha256
            shippingBenchmarkApkSha256=($profileReceipt.apks | Where-Object kind -CEQ 'Shipping/Mobile benchmark' | Select-Object -First 1).sha256
            diagnosticMobileApkSha256=($profileReceipt.apks | Where-Object kind -CEQ 'Diagnostic/Mobile debug' | Select-Object -First 1).sha256
            artifactsAreDevelopmentSigned=$true
        }
        captureCheckoutMetadata=[ordered]@{
            sourceCommits=@($controlCapture.sourceCommit,$profileCapture.sourceCommit | Sort-Object -Unique)
            sourceDirty=$true
            isApkBuildProvenance=$false
            note='Capture runner checkout metadata is not provenance for the reused/derived APKs; profile-build-receipt.json binds those to eafbf82 plus the exact patch.'
        }
        pixelVerifierDiagnostics=[ordered]@{
            status='initial .NET 10 reference failures observed in tool transcript only'
            rawFailureLogRetained=$false
            note='The attempted raw verifier log was zero bytes and is excluded; only the corrected green run and comparison receipt are retained.'
        }
        retainedTrial=[ordered]@{
            runId=$warmupId
            classification='completed control warmup; context only; excluded from ABBA'
            completedFrames=1838
            openingFrames=160
            noABBARunsCurated=$true
        }
        incompleteProfileReplay=[ordered]@{
            status='incomplete; existing 300-second replay observation timeout'
            observedAppPid=17098
            methodNote='Per lead run observation, the harness force-stopped the app after the timeout. The retained native state remains replaying and the app-only logcat has PID 17098. No raw process-exit-status log is claimed.'
            replayPass=$false
            appCrashClaim=$false
        }
        excluded=@('APK/ELF/SPIR-V binaries','signing files or credentials','full device/package/property dumps','personal media','raw synthetic analysis fixture files','actual ABBA A1/B1/B2/A2 runs')
        files=$manifest
    }
    $json=$manifestDocument | ConvertTo-Json -Depth 8
    $stream=[IO.File]::Open((Join-Path $target 'SHA256SUMS.json'),[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try{
        $bytes=[Text.UTF8Encoding]::new($false).GetBytes($json)
        $stream.Write($bytes,0,$bytes.Length)
    }finally{$stream.Dispose()}
    Write-Output "Generated additive SHA256SUMS.json for $($manifest.Count) exact allowlisted files after README.md was present."
}
