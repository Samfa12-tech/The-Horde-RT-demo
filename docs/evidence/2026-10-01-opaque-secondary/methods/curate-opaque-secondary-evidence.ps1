[CmdletBinding()]
param([switch]$ValidateOnly)
$ErrorActionPreference='Stop'
$origin=[IO.Path]::GetFullPath($PSScriptRoot)
$expected='C:\Dev\tmp\horde-opaque-secondary-profile-20261001'
if($origin -ine $expected){throw "Unexpected profile source: $origin"}
$target=[IO.Path]::GetFullPath((Join-Path $origin 'curated-v5'))
if(Test-Path -LiteralPath $target){throw "Refusing preexisting curated target: $target"}
$selected=[Collections.Generic.List[object]]::new()
function Add-File([string]$Source,[string]$Destination,[string]$Category){
    $src=[IO.Path]::GetFullPath($Source)
    if(-not(Test-Path -LiteralPath $src -PathType Leaf)){throw "Missing exact curation input: $src"}
    if([IO.Path]::IsPathRooted($Destination) -or $Destination -match '(^|[/\\])\.\.([/\\]|$)'){throw "Unsafe archive path: $Destination"}
    if(@($script:selected|Where-Object destination -CEQ $Destination).Count){throw "Duplicate curated destination: $Destination"}
    $script:selected.Add([ordered]@{source=$src;destination=$Destination.Replace('\','/');category=$Category})
}
function Add-Relative([string]$Relative,[string]$Destination,[string]$Category){Add-File (Join-Path $origin $Relative) $Destination $Category}

# Human-readable interpretation, portable verifier, and this exact curation recipe.
Add-Relative 'CURATED-README.md' 'README.md' 'report'
Add-Relative 'verify-curated-opaque-secondary.ps1' 'verify-curated-opaque-secondary.ps1' 'verification'
Add-Relative 'curate-opaque-secondary-evidence.ps1' 'methods/curate-opaque-secondary-evidence.ps1' 'method'
Add-Relative 'run-curation-spv-check.ps1' 'methods/run-curation-spv-check.ps1' 'method'
Add-Relative 'curation-tool-reruns/curation-spv-check-receipt.json' 'receipts/curation-spv-check-receipt.json' 'module-provenance'
Add-Relative 'build-receipt.json' 'build-receipt.json' 'build-provenance'
Add-Relative 'opaque-secondary-abba-v2-final-20261001.json' 'opaque-secondary-abba-v2-final-20261001.json' 'aggregate'
Add-Relative 'analyse-opaque-secondary-trial-v2.ps1' 'methods/analyse-opaque-secondary-trial-v2.ps1' 'method'
Add-Relative 'aggregate-opaque-secondary-abba-v2.ps1' 'methods/aggregate-opaque-secondary-abba-v2.ps1' 'method'
Add-Relative 'test-opaque-secondary-parser-v2.ps1' 'methods/test-opaque-secondary-parser-v2.ps1' 'method'
Add-Relative 'test-curated-classification-negative.ps1' 'methods/test-curated-classification-negative.ps1' 'method'
Add-Relative 'parser-fixtures/opaque-secondary-negative-v2d/negative-test-receipt.json' 'tests/v2-negative-test-receipt.json' 'test-receipt'
Add-Relative 'classification-negative-test-receipt-v2.json' 'tests/classification-negative-test-receipt.json' 'test-receipt'
Add-Relative 'curation-failure-receipt.json' 'curation/initial-verification-failures.json' 'curation-history'
Add-Relative 'logs/superseded-first-scope-01.json' 'superseded/v1/first-scope-superseded.json' 'superseded-v1'
Add-Relative 'analyse-opaque-secondary-trial.ps1' 'superseded/v1/analyse-opaque-secondary-trial.ps1' 'superseded-v1'
Add-Relative 'aggregate-opaque-secondary-abba.ps1' 'superseded/v1/aggregate-opaque-secondary-abba.ps1' 'superseded-v1'
Add-Relative 'test-opaque-secondary-parser.ps1' 'superseded/v1/test-opaque-secondary-parser.ps1' 'superseded-v1'
Add-Relative 'parser-fixtures/opaque-secondary-negative-v1/negative-test-receipt.json' 'superseded/v1/negative-test-receipt.json' 'superseded-v1'
foreach($file in @('opaque-secondary-c1-route-20261001-control.analysis.json','opaque-secondary-c1-live-20261001-control.analysis.json')){
    Add-Relative "analyses/$file" "superseded/v1/analyses/$file" 'superseded-v1'
}

# Eight raw run bundles plus strict version-2 per-run analyses.
foreach($slot in @('c1','s1','s2','c2')){
    $role=if($slot -in @('c1','c2')){'control'}else{'isolate'}
    foreach($work in @('route','live')){
        $id="opaque-secondary-$slot-$work-20261001";$sourceRun=Join-Path $origin "phone\$id"
        foreach($item in @(@{src='trial.json';dst="$id/trial.json"},@{src='context-samples.jsonl';dst="$id/context-samples.jsonl"},@{src="$id/benchmark.json";dst="$id/$id/benchmark.json"},@{src="$id/result.json";dst="$id/$id/result.json"})){
            Add-File (Join-Path $sourceRun $item.src) "phone/$($item.dst)" 'raw-run'
        }
        Add-Relative "analyses-v2/$id-$role.v2.analysis.json" "analyses-v2/$id-$role.v2.analysis.json" 'strict-analysis-v2'
    }
}

# Frozen catalog snapshots and compact build/package provenance. APK and ELF payloads stay external.
Add-Relative 'artifacts/control/raygen-variant-catalog.json' 'artifacts/control/raygen-variant-catalog.json' 'catalog'
Add-Relative 'artifacts/control/rayquery-variant-catalog.json' 'artifacts/control/rayquery-variant-catalog.json' 'catalog'
Add-Relative 'artifacts/revised-candidate/build-receipt.json' 'artifacts/revised-candidate/build-receipt.json' 'build-provenance'
Add-Relative 'artifacts/revised-candidate/raygen-variant-catalog.json' 'artifacts/revised-candidate/raygen-variant-catalog.json' 'catalog'
Add-Relative 'artifacts/revised-candidate/rayquery-variant-catalog.json' 'artifacts/revised-candidate/rayquery-variant-catalog.json' 'catalog'
Add-Relative 'artifacts/revised-candidate/source-diff.patch' 'artifacts/revised-candidate/source-diff.patch' 'source-patch'
Add-Relative 'artifacts/revised-candidate/rt_opaque_secondary_investigation.glsl' 'artifacts/revised-candidate/rt_opaque_secondary_investigation.glsl' 'source-guard-header'
# Control receipts and extracts are from the already sealed byte-identical normal control APK.
$primary='C:\Dev\tmp\horde-primary-hit-profile-20261001'
Add-File (Join-Path $primary 'artifacts\control\all-shadow-build-receipt.json') 'receipts/control-canonical-build-receipt.json' 'build-provenance'
Add-File (Join-Path $primary 'artifacts\control\containment.json') 'receipts/control-package-containment.json' 'package-provenance'
Add-File (Join-Path $primary 'audit-primary-hit-packaged-20261001-retry2\packaged-primary-hit-module-audit.json') 'receipts/control-package-module-audit.json' 'module-provenance'
Add-Relative 'artifacts/revised-candidate/build-receipt.json' 'receipts/isolate-build-receipt.json' 'build-provenance'
Add-Relative 'logs/candidate-package-containment-02.json' 'receipts/isolate-package-containment.json' 'package-provenance'
Add-Relative 'logs/lead-sealed-modules.log' 'receipts/isolate-module-sealing-log.txt' 'module-provenance'
Add-Relative 'logs/apk-assets-module-comparison-02.json' 'receipts/assets-and-package-module-comparison.json' 'asset-provenance'
Add-Relative 'logs/catalog-module-comparison-02.json' 'receipts/catalog-module-comparison.json' 'module-provenance'
Add-Relative 'logs/assembleBenchmark-arm64-02.log' 'logs/isolate-arm64-build.log' 'build-log'

# Exact actual APK payload modules: four from each APK, but never the APK or native library.
$controlAuditDir=Join-Path $primary 'audit-primary-hit-packaged-20261001-retry2'
$controlAudit=Get-Content -LiteralPath (Join-Path $controlAuditDir 'packaged-primary-hit-module-audit.json') -Raw|ConvertFrom-Json
$controlArtifact=@($controlAudit.artifacts|Where-Object role -CEQ 'control')
if($controlArtifact.Count -ne 1 -or $controlArtifact[0].apkSha256 -cne 'a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'){throw 'Control module audit does not pin the exact normal control APK'}
$candidateReceipt=Get-Content -LiteralPath (Join-Path $origin 'lead-packaged-modules\receipt.json') -Raw|ConvertFrom-Json
if($candidateReceipt.status -cne 'PASS' -or $candidateReceipt.apkSha256 -cne 'ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce'){throw 'Candidate module receipt does not pin the exact revised isolate APK'}
$moduleArtifacts=[Collections.Generic.List[object]]::new()
$expectedModuleKeys=@('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric','rayquery_compute_shipping_mobile_opaque_fast','rayquery_compute_shipping_mobile_generic_dielectric')
foreach($role in @('control','isolate')){
    $sourceModules=if($role -ceq 'control'){@($controlArtifact[0].modules)}else{@($candidateReceipt.modules)}
    $moduleRows=[Collections.Generic.List[object]]::new()
    foreach($sourceModule in $sourceModules){
        $sha=[string]$sourceModule.sha256
        $catalogRoot=if($role -ceq 'control'){Join-Path $origin 'artifacts\control'}else{Join-Path $origin 'artifacts\revised-candidate'}
        if($role -ceq 'control'){$key=[string]$sourceModule.semanticKey}else{
            $matches=@()
            foreach($catalogFile in @('raygen-variant-catalog.json','rayquery-variant-catalog.json')){$catalog=Get-Content -LiteralPath (Join-Path $catalogRoot $catalogFile) -Raw|ConvertFrom-Json;$matches+=@($catalog.variants|Where-Object {$_.key -in $expectedModuleKeys -and $_.spirvSha256 -ceq $sha})}
            if($matches.Count -ne 1){throw "Module SHA does not map to exactly one expected Shipping/Mobile semantic key: $sha"}
            $key=[string]$matches[0].key
        }
        if($key -notin $expectedModuleKeys){throw "Unexpected semantic key for $role module: $key"}
        if($role -ceq 'control'){$modulePath=Join-Path $controlAuditDir "control-$key.spv";$valLog=Join-Path $controlAuditDir "control-$key-spirv-val.log";$disLog=Join-Path $controlAuditDir "control-$key-spirv-dis.log";$backend=[string]$sourceModule.backend;$words=[int]$sourceModule.words;$atomics=[int]$sourceModule.atomicsIncludingNoResult;$reads=[int]$sourceModule.imageReads;$binding=[bool]$sourceModule.binding22;$valOk=($sourceModule.spirvValExitCode -eq 0);$disOk=($sourceModule.spirvDisExitCode -eq 0)}
        else{$modulePath=Join-Path $origin "lead-packaged-modules\$sha.spv";$valLog=Join-Path $origin "curation-tool-reruns\isolate-$key-spirv-val.log";$disLog=Join-Path $origin "curation-tool-reruns\isolate-$key-spirv-dis.log";$backend=[string]$sourceModule.backend;$words=[int]$sourceModule.words;$atomics=[int]$sourceModule.allAtomicOpcodes;$reads=[int]$sourceModule.opImageRead;$binding=[bool]$sourceModule.binding22;$valOk=($sourceModule.spirvVal -ceq 'PASS');$disOk=($sourceModule.spirvDis -ceq 'PASS')}
        if(-not $valOk -or -not $disOk){throw "SPIR-V tools did not pass for $role/$key"}
        if((Get-FileHash -LiteralPath $modulePath -Algorithm SHA256).Hash.ToLowerInvariant() -cne $sha){throw "Actual module payload hash mismatch $role/$key"}
        if((Get-Item -LiteralPath $modulePath).Length -ne ($words*4)){throw "SPIR-V word count mismatch $role/$key"}
        foreach($proof in @(@{path=$valLog;name='spirv-val'},@{path=$disLog;name='spirv-dis'})){
            if(-not(Test-Path -LiteralPath $proof.path -PathType Leaf)){throw "Missing $($proof.name) proof for $role/$key"}
            $text=[IO.File]::ReadAllText($proof.path);if($text -notmatch '(?m)^exitCode=0\r?$' -or $text -notmatch '(?m)^result=passed(?:;[^\r\n]*)?\r?$'){throw "Invalid $($proof.name) receipt for $role/$key"}
        }
        if($atomics -ne 0 -or $reads -ne 0 -or $binding){throw "Disallowed diagnostic instructions/binding in Shipping module $role/$key"}
        $dest="modules/$role/$key.spv";$valDest="modules/$role/$key-spirv-val.log";$disDest="modules/$role/$key-spirv-dis.log"
        Add-File $modulePath $dest 'actual-packaged-spirv'
        Add-File $valLog $valDest 'spirv-validation-proof'
        Add-File $disLog $disDest 'spirv-disassembly-proof'
        $moduleRows.Add([ordered]@{semanticKey=$key;backend=$backend;sha256=$sha;words=$words;spvPath=$dest;spirvVal='PASS';spirvDis='PASS';valLog=$valDest;disLog=$disDest;atomicInstructions=$atomics;opImageReadCount=$reads;binding22=$binding})
    }
    if($moduleRows.Count -ne 4 -or (@($moduleRows|ForEach-Object semanticKey|Sort-Object) -join ',') -cne (@($expectedModuleKeys|Sort-Object) -join ',')){throw "$role must contain the exact four unique Shipping/Mobile modules"}
    $apkPin=if($role -ceq 'control'){'a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'}else{'ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce'}
    $nativePin=if($role -ceq 'control'){$controlAudit.artifacts[0].nativeLibrarySha256}else{$candidateReceipt.nativeSha256}
    $moduleArtifacts.Add([ordered]@{role=$role;apkSha256=$apkPin;nativeLibrarySha256=$nativePin;artifactBytesArchived=$false;modules=@($moduleRows)})
}
if(@($selected.destination|Sort-Object -Unique).Count -ne $selected.Count){throw 'Curation allowlist contains duplicate destinations'}
$forbidden=[regex]'(?i)\.(apk|so|dll|exe|elf|spvasm|obj|mp4|webm|wav|rgba)$'
foreach($entry in $selected){if($entry.destination -match $forbidden){throw "Forbidden archive file: $($entry.destination)"}}
if($ValidateOnly){Write-Output "Curation preflight PASS: $($selected.Count) explicit files plus README/index/manifests; eight raw runs and eight actual module payloads pinned.";exit 0}

# Destination was proven absent above. Partial output is retained for review on any error.
$null=New-Item -ItemType Directory -Path $target
$copied=[Collections.Generic.List[object]]::new()
try{
    foreach($entry in $selected){
        $dest=Join-Path $target $entry.destination.Replace('/','\')
        if(Test-Path -LiteralPath $dest){throw "Refusing overwrite: $dest"}
        $parent=Split-Path -Parent $dest;if(-not(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent -Force|Out-Null}
        Copy-Item -LiteralPath $entry.source -Destination $dest
        $sourceHash=(Get-FileHash -LiteralPath $entry.source -Algorithm SHA256).Hash.ToLowerInvariant()
        if((Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash.ToLowerInvariant() -cne $sourceHash){throw "Copy hash mismatch: $($entry.destination)"}
        $copied.Add([ordered]@{path=$entry.destination;category=$entry.category;bytes=(Get-Item -LiteralPath $dest).Length;sha256=$sourceHash})
    }
    $pins=[ordered]@{schema=1;classification='nonphysical-opaque-primary-secondary-omission-cost-bound-investigation-only';control=[ordered]@{apkSha256='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033';artifactBytesArchived=$false};isolate=[ordered]@{apkSha256='ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce';artifactBytesArchived=$false};artifacts=@($moduleArtifacts);limitation='APK and native libraries are omitted; APK SHA values are receipt pins and are not rehashed by this archive.'}
    $pinsPath=Join-Path $target 'module-pins.json';$pins|ConvertTo-Json -Depth 10|Set-Content -LiteralPath $pinsPath -Encoding utf8
    $copied.Add([ordered]@{path='module-pins.json';category='module-provenance';bytes=(Get-Item $pinsPath).Length;sha256=(Get-FileHash $pinsPath -Algorithm SHA256).Hash.ToLowerInvariant()})
    $index=[ordered]@{schema=1;origin=$origin;classification='nonphysical-opaque-primary-secondary-omission-cost-bound-investigation-only';groups=@()}
    foreach($category in @($copied.category|Sort-Object -Unique)){$index.groups+=,[ordered]@{category=$category;files=@($copied|Where-Object category -CEQ $category|ForEach-Object path)}}
    $indexPath=Join-Path $target 'curation-index.json';$index|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $indexPath -Encoding utf8
    $copied.Add([ordered]@{path='curation-index.json';category='curation-index';bytes=(Get-Item $indexPath).Length;sha256=(Get-FileHash $indexPath -Algorithm SHA256).Hash.ToLowerInvariant()})
    $allFiles=@(Get-ChildItem -LiteralPath $target -Recurse -File|Where-Object Name -CNE 'manifest.json')
    $manifest=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');classification='nonphysical-opaque-primary-secondary-omission-cost-bound-investigation-only';sourceCommit='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95';controlApkSha256='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033';isolateApkSha256='ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce';apkBytesArchived=$false;nativeLibraryBytesArchived=$false;files=@($allFiles|ForEach-Object {[ordered]@{path=$_.FullName.Substring($target.Length+1).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}}|Sort-Object path);fileCount=$allFiles.Count;exclusions=@('control and isolate APK binaries','native .so/ELF payloads','full SPIR-V disassemblies','build directory and unrelated logs','superseded first-scope APK and binary outputs')}
    $manifestPath=Join-Path $target 'manifest.json';$manifest|ConvertTo-Json -Depth 10|Set-Content -LiteralPath $manifestPath -Encoding utf8
    & (Join-Path $target 'verify-curated-opaque-secondary.ps1') -Root $target
    Write-Output "Curation complete: $($manifest.fileCount) manifest files; eight raw runs, eight SPIR-V modules; APK/native binaries omitted."
}catch{throw "Curation stopped with a partial external packet; preserve for review. $($_.Exception.Message)"}
