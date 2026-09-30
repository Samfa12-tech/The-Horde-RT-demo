[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Root)
$ErrorActionPreference='Stop'
$rootPath=(Resolve-Path -LiteralPath $Root -ErrorAction Stop).Path
$expectedHead='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'
$expectedApks=@{control='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033';reference='40d0f759dca40ff8433ce4aca19156a135a9146a960f115f4cdbb78d3db7cf9e'}
$vulkanBin='C:\VulkanSDK\1.4.350.0\Bin'
$val=Join-Path $vulkanBin 'spirv-val.exe'; $dis=Join-Path $vulkanBin 'spirv-dis.exe'
foreach($tool in @($val,$dis)){if(-not(Test-Path -LiteralPath $tool -PathType Leaf)){throw "Missing required SPIR-V tool: $tool"}}
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Read-Json([string]$Relative){Get-Content -LiteralPath (Join-Path $rootPath $Relative) -Raw|ConvertFrom-Json -DateKind String}
function Assert-Equal($Actual,$Expected,[string]$Label){if($Actual -cne $Expected){throw "$Label expected '$Expected', found '$Actual'"}}
function Get-Canonical($Value){ConvertTo-Json -InputObject $Value -Depth 24 -Compress}
function Copy-Relative([string]$Relative,[string]$TempRoot){
    $src=Join-Path $rootPath $Relative
    if(-not(Test-Path -LiteralPath $src -PathType Leaf)){throw "Missing archived evidence input: $Relative"}
    $dst=Join-Path $TempRoot $Relative
    $parent=Split-Path -Parent $dst
    if(-not(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent -Force|Out-Null}
    Copy-Item -LiteralPath $src -Destination $dst
}

$build=Read-Json 'build-receipt.json'
Assert-Equal $build.worktree.head $expectedHead 'source HEAD'
Assert-Equal $build.control.apkSha256 $expectedApks.control 'control APK pin'
Assert-Equal $build.reference.apkSha256 $expectedApks.reference 'reference APK pin'
$finalPath=Join-Path $rootPath 'primary-hit-abba-final-reviewed-20261001.json'
if(-not(Test-Path -LiteralPath $finalPath -PathType Leaf)){throw 'Missing final reviewed 12-run aggregate.'}
$finalAggregate=Get-Content -LiteralPath $finalPath -Raw|ConvertFrom-Json -DateKind String
Assert-Equal $finalAggregate.status 'all-12-analyzed' 'final aggregate state'
Assert-Equal $finalAggregate.analysisCount 12 'final aggregate analysis count'
Assert-Equal $finalAggregate.expectedAnalysisCount 12 'final aggregate expected analysis count'
Assert-Equal $finalAggregate.controlApkSha256 $expectedApks.control 'aggregate control APK identity'
Assert-Equal $finalAggregate.referenceApkSha256 $expectedApks.reference 'aggregate reference APK identity'

$sourcePatch=Join-Path $rootPath 'artifacts\primary-hit-probe\source-diff.patch'
$sourceHeader=Join-Path $rootPath 'artifacts\primary-hit-probe\rt_primary_hit_reference.glsl'
Assert-Equal (Get-FileHash -LiteralPath $sourcePatch -Algorithm SHA256).Hash.ToLowerInvariant() '4500f823836f19123fd85b38fb4dd0220d11b96c84decb4e7b57280c0c74a277' 'source patch SHA'
Assert-Equal (Get-FileHash -LiteralPath $sourceHeader -Algorithm SHA256).Hash.ToLowerInvariant() 'bca3e4bb9f7c7c4aec176c38dcef5f988f94434d078db1cc77ca58705ab9cadb' 'reference header SHA'
$asset=Read-Json 'artifacts\primary-hit-probe\asset-shader-comparison.json'
Assert-Equal $asset.inputs.baselineApkSha256 $expectedApks.control 'asset comparison control identity'
Assert-Equal $asset.inputs.candidateApkSha256 $expectedApks.reference 'asset comparison reference identity'
Assert-Equal $asset.assetSummary.baselineCount 53 'baseline asset count'
Assert-Equal $asset.assetSummary.candidateCount 53 'candidate asset count'
Assert-Equal $asset.assetSummary.identicalCount 53 'identical asset count'
Assert-Equal $asset.assetSummary.changedCount 0 'changed asset count'

$auditPath='audit-primary-hit-packaged-20261001-retry2\packaged-primary-hit-module-audit.json'
$audit=Read-Json $auditPath
Assert-Equal $audit.classification 'nonphysical-primary-hit-reference-investigation-only' 'module audit classification'
Assert-Equal $audit.auditRules.atomicMatcherIncludesNoResultOpcodes $true 'no-result atomic matching rule'
Assert-Equal $audit.auditRules.moduleCountEach 4 'module count per artifact'
Assert-Equal $audit.auditRules.shippingAtomicCount 0 'Shipping atomics'
Assert-Equal $audit.auditRules.opImageReadCount 0 'Shipping image reads'
Assert-Equal $audit.auditRules.binding22 $false 'Shipping diagnostic binding'
Assert-Equal (Get-FileHash -LiteralPath (Join-Path $rootPath 'audit-primary-hit-packaged-modules.ps1') -Algorithm SHA256).Hash.ToLowerInvariant() $audit.scriptSha256 'packaged-audit script identity'
$expectedModuleKeys=@('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric','rayquery_compute_shipping_mobile_opaque_fast','rayquery_compute_shipping_mobile_generic_dielectric')
Assert-Equal @($audit.artifacts).Count 2 'exact packaged artifact count'
$artifactRoles=@($audit.artifacts | ForEach-Object { [string]$_.role })
if(@($artifactRoles | Sort-Object -Unique).Count -ne 2 -or
    (@($artifactRoles | Sort-Object) -join ',') -cne 'control,reference'){
    throw "Packaged audit must contain exactly one control and one reference artifact; found $($artifactRoles -join ',')."
}
if(@($audit.changedSemanticKeys | Sort-Object -Unique).Count -ne 4 -or
    (@($audit.changedSemanticKeys | Sort-Object) -join ',') -cne (@($expectedModuleKeys | Sort-Object) -join ',')){
    throw 'Packaged audit changed-key set does not match the exact four Shipping/Mobile variants.'
}
$valDisRows=0
foreach($artifact in $audit.artifacts){
    $role=[string]$artifact.role
    if($role -cnotin @('control','reference')){throw "Unexpected artifact role in packaged audit: $role"}
    Assert-Equal $artifact.apkSha256 $expectedApks[$role] "$role actual APK SHA"
    $artifactPath=if($role -ceq 'control'){'artifacts\control'}else{'artifacts\primary-hit-probe'}
    $containmentName=if($role -ceq 'control'){'containment.json'}else{'package-containment.json'}
    $containment=Read-Json "$artifactPath\$containmentName"
    Assert-Equal $containment.instrumentation 'Shipping' "$role containment instrumentation"
    Assert-Equal $containment.quality 'Mobile' "$role containment quality"
    Assert-Equal $containment.arm64Sha256 $artifact.nativeLibrarySha256 "$role native library identity"
    if(@($containment.packaged.semanticKeys | Sort-Object -Unique).Count -ne 4 -or
        (@($containment.packaged.semanticKeys | Sort-Object) -join ',') -cne (@($expectedModuleKeys | Sort-Object) -join ',')){
        throw "$role package semantic-key set does not match the four expected Shipping/Mobile modules."
    }
    Assert-Equal @($artifact.modules).Count 4 "$role audited module count"
    $artifactKeys=@($artifact.modules | ForEach-Object { [string]$_.semanticKey })
    if(@($artifactKeys | Sort-Object -Unique).Count -ne 4 -or
        (@($artifactKeys | Sort-Object) -join ',') -cne (@($expectedModuleKeys | Sort-Object) -join ',')){
        throw "$role audit must contain four unique expected semantic keys; found $($artifactKeys -join ',')."
    }
    foreach($module in $artifact.modules){
        $key=[string]$module.semanticKey
        if($key -notin $expectedModuleKeys){throw "Unexpected packaged semantic key: $key"}
        $spvRel="audit-primary-hit-packaged-20261001-retry2\$role-$key.spv"
        $spv=Join-Path $rootPath $spvRel
        $sha=(Get-FileHash -LiteralPath $spv -Algorithm SHA256).Hash.ToLowerInvariant()
        Assert-Equal $sha $module.sha256 "$role/$key SPIR-V SHA"
        $match=@($containment.packaged.modules|Where-Object sha256 -CEQ $sha)
        Assert-Equal $match.Count 1 "$role/$key containment offset match"
        Assert-Equal $match[0].words $module.words "$role/$key containment word count"
        $catalogName=if($module.backend -ceq 'RayTracingPipeline'){'raygen-variant-catalog.json'}else{'rayquery-variant-catalog.json'}
        $catalog=Read-Json "$artifactPath\$catalogName"
        $catalogRows=@($catalog.variants|Where-Object { $_.key -ceq $key -and $_.spirvSha256 -ceq $sha })
        Assert-Equal $catalogRows.Count 1 "$role/$key frozen catalog module join"
        $valOutput=& $val --target-env vulkan1.2 $spv 2>&1
        $valExit=$LASTEXITCODE
        if($valExit -ne 0){throw "spirv-val failed for $role/$key exit=$valExit"}
        $tmpDis=Join-Path ([IO.Path]::GetTempPath()) ("primary-hit-archive-"+[guid]::NewGuid().ToString('N')+'.spvasm')
        try{
            $disOutput=& $dis $spv -o $tmpDis 2>&1
            $disExit=$LASTEXITCODE
            if($disExit -ne 0 -or -not(Test-Path -LiteralPath $tmpDis -PathType Leaf)){throw "spirv-dis failed for $role/$key exit=$disExit"}
            $text=[IO.File]::ReadAllText($tmpDis)
            $atomics=[regex]::Matches($text,'(?m)^\s*(?:%\S+\s*=\s*)?OpAtomic\w+\b').Count
            $imageReads=[regex]::Matches($text,'(?m)^\s*(?:%\S+\s*=\s*)?OpImageRead\b').Count
            $binding22=[regex]::IsMatch($text,'(?m)^\s*OpDecorate\s+%\S+\s+Binding\s+22\s*$')
            $queries=[regex]::Matches($text,'\bOpRayQueryInitializeKHR\b').Count
            $instructionCount=[regex]::Matches($text,'(?m)^\s*(?:%\S+\s*=\s*)?Op\w+\b').Count
        }finally{if(Test-Path -LiteralPath $tmpDis -PathType Leaf){Remove-Item -LiteralPath $tmpDis}}
        Assert-Equal $atomics 0 "$role/$key atomic instruction count"
        Assert-Equal $imageReads 0 "$role/$key OpImageRead count"
        Assert-Equal $binding22 $false "$role/$key binding 22 presence"
        Assert-Equal $queries $module.rayQueryInitializations "$role/$key query initialization count"
        Assert-Equal $instructionCount $module.disassemblyInstructionCount "$role/$key disassembly instruction count"
        $valDisRows++
    }
}
Assert-Equal $valDisRows 8 'independently revalidated SPIR-V module count'

$runs=@()
foreach($block in @('c1','p1','p2','c2')){
    $role=if($block -in @('c1','c2')){'control'}else{'reference'}
    foreach($work in @('route','high','live')){
        $id="primary-hit-$block-$work-20261001"
        $runRoot=Join-Path $rootPath "phone\$id"
        foreach($path in @("$runRoot\trial.json","$runRoot\context-samples.jsonl","$runRoot\launch.log","$runRoot\pull.log",
            "$runRoot\$id\benchmark.json","$runRoot\$id\result.json")){
            if(-not(Test-Path -LiteralPath $path -PathType Leaf)){throw "Missing exact phone evidence for ${id}: $path"}
        }
        $analysisPath=Join-Path $rootPath "analyses\$id-$role.analysis.json"
        if(-not(Test-Path -LiteralPath $analysisPath -PathType Leaf)){throw "Missing strict per-run analysis: $analysisPath"}
        $a=Get-Content -LiteralPath $analysisPath -Raw|ConvertFrom-Json -DateKind String
        Assert-Equal $a.runId $id "$id analysis ID"
        Assert-Equal $a.artifactRole $role "$id analysis role"
        Assert-Equal $a.integrity 'PASS' "$id analysis integrity"
        $runs+=,[pscustomobject]@{id=$id;role=$role;analysisPath=$analysisPath;work=$work}
    }
}
Assert-Equal $runs.Count 12 'exact run count'
foreach($block in @('c1','p1','c2')){
    $install=Join-Path $rootPath "installs\primary-hit-$block-20261001"
    foreach($name in @('install.log','pull.log','receipt.json')){if(-not(Test-Path -LiteralPath (Join-Path $install $name) -PathType Leaf)){throw "Missing install evidence: $install\$name"}}
}

# Re-run the unchanged strict individual parser against the archived raw ledgers,
# then rerun the unchanged aggregate and compare canonical JSON values.
$tempRoot=Join-Path ([IO.Path]::GetTempPath()) ("horde-primary-hit-verify-"+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tempRoot|Out-Null
try{
    foreach($relative in @('analyse-primary-hit-trial.ps1','aggregate-primary-hit-abba.ps1','build-receipt.json')){Copy-Relative $relative $tempRoot}
    foreach($relative in @(
        'artifacts\control\all-shadow-build-receipt.json','artifacts\control\containment.json',
        'artifacts\control\raygen-variant-catalog.json','artifacts\control\rayquery-variant-catalog.json',
        'artifacts\primary-hit-probe\asset-shader-comparison.json','artifacts\primary-hit-probe\build-receipt.json',
        'artifacts\primary-hit-probe\package-containment.json','artifacts\primary-hit-probe\raygen-variant-catalog.json',
        'artifacts\primary-hit-probe\rayquery-variant-catalog.json')){Copy-Relative $relative $tempRoot}
    foreach($run in $runs){
        $id=$run.id; $src=Join-Path $rootPath "phone\$id"; $dst=Join-Path $tempRoot "phone\$id"
        foreach($rel in @('trial.json','context-samples.jsonl','launch.log','pull.log',"$id\benchmark.json","$id\result.json")){
            $srcFile=Join-Path $src $rel; $dstFile=Join-Path $dst $rel
            if(-not(Test-Path -LiteralPath $srcFile -PathType Leaf)){throw "Missing raw trial input during strict replay: $srcFile"}
            $parent=Split-Path -Parent $dstFile;if(-not(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent -Force|Out-Null}
            Copy-Item -LiteralPath $srcFile -Destination $dstFile
        }
        $parser=Join-Path $tempRoot 'analyse-primary-hit-trial.ps1'
        & $parser -RunPath $dst -ArtifactRole $run.role|Out-Null
        $rerunPath=Join-Path $tempRoot "analyses\$id-$($run.role).analysis.json"
        $old=Get-Content -LiteralPath $run.analysisPath -Raw|ConvertFrom-Json -DateKind String
        $new=Get-Content -LiteralPath $rerunPath -Raw|ConvertFrom-Json -DateKind String
        if((Get-Canonical $old) -cne (Get-Canonical $new)){throw "Canonical strict-analysis mismatch for $id"}
    }
    $recomputed=Join-Path $tempRoot 'aggregate-recomputed.json'
    & (Join-Path $tempRoot 'aggregate-primary-hit-abba.ps1') -OutputPath $recomputed|Out-Null
    $expected=Get-Content -LiteralPath $finalPath -Raw|ConvertFrom-Json -DateKind String
    $actual=Get-Content -LiteralPath $recomputed -Raw|ConvertFrom-Json -DateKind String
    $expected.PSObject.Properties.Remove('createdUtc')|Out-Null
    $actual.PSObject.Properties.Remove('createdUtc')|Out-Null
    foreach($state in $expected.individualRunStates){$state.path=[IO.Path]::GetFileName([string]$state.path)}
    foreach($state in $actual.individualRunStates){$state.path=[IO.Path]::GetFileName([string]$state.path)}
    if((Get-Canonical $expected) -cne (Get-Canonical $actual)){throw 'Canonical final aggregate differs from independent 12-run recomputation.'}
}finally{
    $tempFull=[IO.Path]::GetFullPath($tempRoot)
    $tempPrefix=[IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if(-not $tempFull.StartsWith($tempPrefix,[StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($tempFull) -notmatch '^horde-primary-hit-verify-[0-9a-f]{32}$'){
        throw "Refusing cleanup outside this verifier's unique temp directory: $tempFull"
    }
    if(Test-Path -LiteralPath $tempFull){Remove-Item -LiteralPath $tempFull -Recurse -Force}
}
Write-Output 'Primary-hit archive verification passed: 8 packaged SPIR-V modules independently val/dis checked; 12 raw trials strictly reanalyzed; canonical 12-run aggregate reproduced.'
