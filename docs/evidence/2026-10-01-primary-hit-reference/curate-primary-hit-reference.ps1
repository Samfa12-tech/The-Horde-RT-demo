[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Destination,
    [switch]$ValidateOnly
)
$ErrorActionPreference='Stop'
$origin=[IO.Path]::GetFullPath($PSScriptRoot)
$expectedOrigin='C:\Dev\tmp\horde-primary-hit-profile-20261001'
$engineering='C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.6.1-engineering-pass'
$expectedDestination=[IO.Path]::GetFullPath((Join-Path $engineering 'docs\evidence\2026-10-01-primary-hit-reference'))
if($origin -ine $expectedOrigin){throw "Unexpected evidence origin: $origin"}
$target=[IO.Path]::GetFullPath($Destination)
if($target -ine $expectedDestination){throw "Destination must be the exact primary-hit evidence directory: $expectedDestination"}

function Assert-NoReparse([string]$Path,[string]$StopAt){
    $cursor=[IO.Path]::GetFullPath($Path)
    $stop=[IO.Path]::GetFullPath($StopAt)
    while($cursor.Length -ge $stop.Length){
        if(Test-Path -LiteralPath $cursor){
            $item=Get-Item -LiteralPath $cursor -Force
            if(($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0){throw "Reparse point is not allowed in evidence path: $cursor"}
        }
        if($cursor -ieq $stop){return}
        $next=Split-Path -Parent $cursor
        if(-not $next -or $next -ieq $cursor){break}
        $cursor=$next
    }
    throw "Path is not inside the approved evidence tree: $Path"
}
Assert-NoReparse -Path $target -StopAt (Join-Path $engineering 'docs\evidence')
if(Test-Path -LiteralPath $target){throw "Refusing existing evidence destination: $target"}

$selected=[Collections.Generic.List[string]]::new()
foreach($name in @(
    'aggregate-primary-hit-abba.ps1','analyse-primary-hit-trial.ps1','audit-primary-hit-packaged-modules.ps1',
    'build-receipt.json','curate-primary-hit-reference.ps1','install-member.ps1','run-abba.ps1',
    'run-shipping-trial.ps1','test-analyse-primary-hit-trial.ps1','verify-primary-hit-archive.ps1',
    'primary-hit-abba-final-reviewed-20261001.json',
    'install-c1-20261001.log','install-p1-20261001.log','install-c2-20261001.log',
    'logs/assembleBenchmark-arm64.log','logs/compute-generator-check.log','logs/compute-generator-write.log',
    'logs/pipeline-adapter-check.log','logs/raygen-checkcatalog.log','logs/raygen-checkcatalog-retry.log',
    'logs/raygen-freeze.log','logs/primary-hit-packaged-module-audit-initial-failure-transcript-20261001.txt',
    'logs/primary-hit-packaged-module-audit-retry1-20261001.log','logs/primary-hit-packaged-module-audit-retry2-20261001.log',
    'parser-fixtures/synthetic-only-v4/synthetic-regression-receipt.json'
)){$selected.Add($name)}

foreach($block in @('c1','p1','p2','c2')){
    $role=if($block -in @('c1','c2')){'control'}else{'reference'}
    foreach($work in @('route','high','live')){
        $id="primary-hit-$block-$work-20261001"
        $selected.Add("$id.log")
        $selected.Add("analyses/$id-$role.analysis.json")
        foreach($name in @('context-samples.jsonl','launch.log','pull.log','trial.json')){$selected.Add("phone/$id/$name")}
        foreach($name in @('benchmark.json','result.json')){$selected.Add("phone/$id/$id/$name")}
    }
}
foreach($block in @('c1','p1','c2')){
    foreach($name in @('install.log','pull.log','receipt.json')){$selected.Add("installs/primary-hit-$block-20261001/$name")}
}
foreach($name in @('all-shadow-build-receipt.json','containment.json','raygen-variant-catalog.json','rayquery-variant-catalog.json')){
    $selected.Add("artifacts/control/$name")
}
foreach($name in @('asset-shader-comparison.json','build-receipt.json','package-containment.json','raygen-variant-catalog.json',
    'rayquery-variant-catalog.json','rt_primary_hit_reference.glsl','source-diff.patch')){
    $selected.Add("artifacts/primary-hit-probe/$name")
}
$selected.Add('audit-primary-hit-packaged-20261001-retry2/packaged-primary-hit-module-audit.json')
foreach($artifact in @('control','reference')){
    foreach($key in @('rayquery_compute_shipping_mobile_opaque_fast','rayquery_compute_shipping_mobile_generic_dielectric',
        'shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric')){
        $selected.Add("audit-primary-hit-packaged-20261001-retry2/$artifact-$key.spv")
        $selected.Add("audit-primary-hit-packaged-20261001-retry2/$artifact-$key-spirv-val.log")
        $selected.Add("audit-primary-hit-packaged-20261001-retry2/$artifact-$key-spirv-dis.log")
    }
}
if(@($selected | Sort-Object -Unique).Count -ne $selected.Count){throw 'Explicit allowlist contains duplicate paths.'}
$forbidden=[regex]'(?i)\.(apk|so|elf|png|mp4|webm|wav|rgba|spvasm|obj|zip)$'
foreach($path in $selected){if($path -match $forbidden){throw "Forbidden archive payload in allowlist: $path"}}
$missing=[Collections.Generic.List[string]]::new()
foreach($path in $selected){if(-not(Test-Path -LiteralPath (Join-Path $origin $path) -PathType Leaf)){$missing.Add($path)}}

$runStatuses=@()
foreach($block in @('c1','p1','p2','c2')){
    $role=if($block -in @('c1','c2')){'control'}else{'reference'}
    foreach($work in @('route','high','live')){
        $id="primary-hit-$block-$work-20261001"
        $analysis=Join-Path $origin "analyses/$id-$role.analysis.json"
        $status=if(Test-Path -LiteralPath $analysis -PathType Leaf){'analysis-present'}else{'analysis-pending'}
        $runStatuses+=,[pscustomobject]@{runId=$id;role=$role;status=$status}
    }
}
$finalAggregate=Join-Path $origin 'primary-hit-abba-final-reviewed-20261001.json'
if(-not(Test-Path -LiteralPath $finalAggregate -PathType Leaf)){
    if(-not $missing.Contains('primary-hit-abba-final-reviewed-20261001.json')){$missing.Add('primary-hit-abba-final-reviewed-20261001.json')}
}else{
    $aggregate=Get-Content -LiteralPath $finalAggregate -Raw|ConvertFrom-Json
    if($aggregate.status -cne 'all-12-analyzed' -or $aggregate.analysisCount -ne 12 -or $aggregate.expectedAnalysisCount -ne 12){
        throw 'Final reviewed aggregate must be all-12-analyzed with exact 12/12 count.'
    }
}

Write-Output "Explicit allowlist entries: $($selected.Count); planned workload records: $($runStatuses.Count)."
$runStatuses | Format-Table -AutoSize | Out-String | Write-Output
if($missing.Count){
    Write-Output "Not ready; missing $($missing.Count) exact required inputs (no curation performed):"
    $missing | ForEach-Object { Write-Output "  $_" }
    if($ValidateOnly){exit 2}
    throw 'Primary-hit evidence packet is incomplete; refusing curation.'
}
if($ValidateOnly){Write-Output 'Preflight passed; no files written.'; exit 0}

$verifier=Join-Path $origin 'verify-primary-hit-archive.ps1'
& $verifier -Root $origin
$null=New-Item -ItemType Directory -Path $target
$copied=[Collections.Generic.List[object]]::new()
try{
    foreach($path in $selected){
        $source=Join-Path $origin $path
        $dest=Join-Path $target $path
        if(Test-Path -LiteralPath $dest){throw "Refusing overwrite of allowlisted output: $dest"}
        $null=New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force
        Copy-Item -LiteralPath $source -Destination $dest
        $sha=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
        if((Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash.ToLowerInvariant() -cne $sha){throw "Copy hash mismatch: $path"}
        $copied.Add([ordered]@{path=$path;bytes=(Get-Item -LiteralPath $dest).Length;sha256=$sha})
    }
    $readme=Join-Path $target 'README.md'
    if(Test-Path -LiteralPath $readme -PathType Leaf){
        $copied.Add([ordered]@{path='README.md';bytes=(Get-Item -LiteralPath $readme).Length;sha256=(Get-FileHash $readme -Algorithm SHA256).Hash.ToLowerInvariant()})
    }
    & $verifier -Root $target
    $manifestPath=Join-Path $target 'manifest.json'
    if(Test-Path -LiteralPath $manifestPath){throw 'Refusing to overwrite archive manifest.'}
    $manifest=[ordered]@{schema=1;classification='nonphysical-primary-hit-reference-investigation-only';origin=$origin;destination=$target
        sourceHead='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95';controlApkSha256='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'
        referenceApkSha256='40d0f759dca40ff8433ce4aca19156a135a9146a960f115f4cdbb78d3db7cf9e';files=@($copied);count=$copied.Count
        exclusions=@('sealed APKs','native ELF libraries','full source worktree','SPIR-V disassemblies','synthetic fixture reports beyond one regression receipt')}
    $manifest|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $manifestPath -Encoding utf8
    Write-Output "Curated and hash-checked $($copied.Count) explicit evidence files; offline archive verification passed."
}catch{
    throw "Curation stopped after a partial target was created. Preserve it for review; no automatic cleanup or overwrite is attempted. $($_.Exception.Message)"
}
