param([Parameter(Mandatory=$true)][string]$WorkDirectory,
      [string]$VulkanBin='C:/VulkanSDK/1.4.350.0/Bin')
$ErrorActionPreference='Stop'
$evidence=$PSScriptRoot
$work=[IO.Path]::GetFullPath($WorkDirectory)
if(Test-Path -LiteralPath $work){throw "Refusing existing offline output: $work"}
$null=New-Item -ItemType Directory -Path $work
foreach($name in @('analyse-all-shadow-trial.ps1','aggregate-all-shadow-abba.ps1','build-receipt.json')){
    Copy-Item -LiteralPath (Join-Path $evidence $name) -Destination (Join-Path $work $name)
}
foreach($name in @('phone','artifacts')){
    Copy-Item -LiteralPath (Join-Path $evidence $name) -Destination (Join-Path $work $name) -Recurse
}
$moduleResults=[Collections.Generic.List[object]]::new()
$audit=Get-Content (Join-Path $evidence 'packaged-module-audit.json') -Raw|ConvertFrom-Json
foreach($artifact in $audit.artifacts){
    foreach($module in $artifact.modules){
        $file=Join-Path $evidence "artifacts/$($artifact.kind)/actual-packaged-modules-audit2/$($module.key).spv"
        $hash=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
        if($hash -cne $module.sha256 -or (Get-Item -LiteralPath $file).Length -ne 4*$module.words){throw "Actual module byte mismatch: $file"}
        & (Join-Path $VulkanBin 'spirv-val.exe') --target-env vulkan1.2 $file
        if($LASTEXITCODE -ne 0){throw "SPIR-V validation failed: $file"}
        $asm=Join-Path $work "$($artifact.kind)-$($module.key).spvasm"
        & (Join-Path $VulkanBin 'spirv-dis.exe') $file -o $asm
        if($LASTEXITCODE -ne 0){throw "Disassembly failed: $file"}
        $text=[IO.File]::ReadAllText($asm)
        $atoms=[regex]::Matches($text,'(?m)^\s*(?:%\S+\s*=\s*)?OpAtomic\w+\b').Count
        $reads=[regex]::Matches($text,'(?m)^\s*(?:%\S+\s*=\s*)?OpImageRead\b').Count
        $binding=[regex]::IsMatch($text,'(?m)^\s*OpDecorate\s+%\S+\s+Binding\s+22\s*$')
        if($atoms -ne 0 -or $reads -ne 0 -or $binding){throw "Shipping diagnostic overhead found: $file"}
        $moduleResults.Add([ordered]@{role=$artifact.kind;key=$module.key;sha256=$hash;validation='PASS';atomics=$atoms;imageReads=$reads;binding22=$binding})
    }
}
if($moduleResults.Count -ne 8){throw 'Expected eight actual packaged modules'}
$checks=[Collections.Generic.List[object]]::new()
foreach($block in @('c1','s1','s2','c2')){
    $role=if($block.StartsWith('c')){'control'}else{'isolate'}
    foreach($case in @('route','high','live')){
        $id="all-shadow-$block-$case-20261001"
        & (Join-Path $work 'analyse-all-shadow-trial.ps1') -RunPath (Join-Path $work "phone/$id") -ArtifactRole $role
        $name="$id-$role.analysis.json"
        $actual=Get-Content (Join-Path $work "analyses/$name") -Raw|ConvertFrom-Json -DateKind String|ConvertTo-Json -Depth 30 -Compress
        $expected=Get-Content (Join-Path $evidence "analyses/$name") -Raw|ConvertFrom-Json -DateKind String|ConvertTo-Json -Depth 30 -Compress
        if($actual -cne $expected){throw "Recomputed analysis differs: $id"}
        $checks.Add([ordered]@{run=$id;recomputedAnalysis='exact canonical JSON match'})
    }
}
$aggregate=Join-Path $work 'recomputed-aggregate.json'
& (Join-Path $work 'aggregate-all-shadow-abba.ps1') -OutputPath $aggregate
$actual=Get-Content $aggregate -Raw|ConvertFrom-Json -DateKind String
$expected=Get-Content (Join-Path $evidence 'all-shadow-abba-final-reviewed-20261001.json') -Raw|ConvertFrom-Json -DateKind String
foreach($key in @('status','analysisCount','perRunTable','perWorkloadSummaries')){
    if(($actual.$key|ConvertTo-Json -Depth 30 -Compress) -cne ($expected.$key|ConvertTo-Json -Depth 30 -Compress)){throw "Aggregate recomputation differs: $key"}
}
[ordered]@{schema=1;status='PASS';deviceOperation=$false;originalEvidenceReadOnly=$true;modules=@($moduleResults);analyses=@($checks);aggregate='exact per-run and per-workload values; creation time/output paths excluded'}|
    ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $work 'offline-verification.json')
Write-Output 'Offline verification PASS:8 actual modules,12 raw-ledger reanalyses, aggregate recomputation.'
