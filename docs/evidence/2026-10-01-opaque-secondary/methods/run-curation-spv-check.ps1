param()
$ErrorActionPreference='Stop'
$root='C:\Dev\tmp\horde-opaque-secondary-profile-20261001'
$output=Join-Path $root 'curation-tool-reruns'
if(Test-Path -LiteralPath $output){throw "Refusing existing tool rerun directory: $output"}
$null=New-Item -ItemType Directory -Path $output
$disassembly=Join-Path $output 'disassembly-not-archived'
$null=New-Item -ItemType Directory -Path $disassembly
$val='C:\VulkanSDK\1.4.350.0\Bin\spirv-val.exe'
$dis='C:\VulkanSDK\1.4.350.0\Bin\spirv-dis.exe'
foreach($tool in @($val,$dis)){if(-not(Test-Path -LiteralPath $tool -PathType Leaf)){throw "Missing SPIR-V tool: $tool"}}
$modules=[Collections.Generic.List[object]]::new()
$controlDir='C:\Dev\tmp\horde-primary-hit-profile-20261001\audit-primary-hit-packaged-20261001-retry2'
$controlAudit=Get-Content -LiteralPath (Join-Path $controlDir 'packaged-primary-hit-module-audit.json') -Raw|ConvertFrom-Json
$control=@($controlAudit.artifacts|Where-Object role -CEQ 'control')
if($control.Count -ne 1 -or $control[0].apkSha256 -cne 'a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'){throw 'Control package audit pin mismatch'}
foreach($m in $control[0].modules){$modules.Add([ordered]@{role='control';key=$m.semanticKey;sha=$m.sha256;spv=(Join-Path $controlDir "control-$($m.semanticKey).spv")})}
$candidateDir=Join-Path $root 'lead-packaged-modules'
$candidateReceipt=Get-Content -LiteralPath (Join-Path $candidateDir 'receipt.json') -Raw|ConvertFrom-Json
if($candidateReceipt.apkSha256 -cne 'ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce'){throw 'Candidate package audit pin mismatch'}
foreach($m in $candidateReceipt.modules){
    $matches=@()
    foreach($catalogPath in @('artifacts\revised-candidate\raygen-variant-catalog.json','artifacts\revised-candidate\rayquery-variant-catalog.json')){
        $catalog=Get-Content -LiteralPath (Join-Path $root $catalogPath) -Raw|ConvertFrom-Json
        $matches+=@($catalog.variants|Where-Object {$_.key -in @('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric','rayquery_compute_shipping_mobile_opaque_fast','rayquery_compute_shipping_mobile_generic_dielectric') -and $_.spirvSha256 -ceq $m.sha256})
    }
    if($matches.Count -ne 1){throw "Candidate module key is not unique: $($m.sha256)"}
    $modules.Add([ordered]@{role='isolate';key=$matches[0].key;sha=$m.sha256;spv=(Join-Path $candidateDir "$($m.sha256).spv")})
}
if($modules.Count -ne 8){throw "Expected exactly eight packaged modules, found $($modules.Count)"}
foreach($m in $modules){
    $hash=(Get-FileHash -LiteralPath $m.spv -Algorithm SHA256).Hash.ToLowerInvariant()
    if($hash -cne $m.sha){throw "Actual packaged module SHA mismatch $($m.role)/$($m.key)"}
    $valLog=Join-Path $output "$($m.role)-$($m.key)-spirv-val.log"
    $valOutput=& $val --target-env vulkan1.2 $m.spv 2>&1
    $valExit=$LASTEXITCODE
    $valResult=if($valExit -eq 0){'passed'}else{'failed'}
    "moduleSha256=$hash`ncommand=spirv-val --target-env vulkan1.2 <module>`nexitCode=$valExit`nresult=$valResult`noutput=$($valOutput -join ' ')"|Set-Content -LiteralPath $valLog -Encoding utf8
    if($valExit -ne 0){throw "spirv-val failed for $($m.role)/$($m.key)"}
    $disFile=Join-Path $disassembly ("$($m.role)-$($m.key)-"+[guid]::NewGuid().ToString('N')+'.spvasm')
    $disLog=Join-Path $output "$($m.role)-$($m.key)-spirv-dis.log"
    $disOutput=& $dis $m.spv -o $disFile 2>&1
    $disExit=$LASTEXITCODE
    $disResult=if($disExit -eq 0 -and (Test-Path -LiteralPath $disFile -PathType Leaf)){'passed'}else{'failed'}
    "moduleSha256=$hash`ncommand=spirv-dis <module> -o <temporary>`nexitCode=$disExit`nresult=$disResult`noutput=$($disOutput -join ' ')`nfullDisassemblyRetained=$true`narchivePolicy=not-included"|Set-Content -LiteralPath $disLog -Encoding utf8
    if($disExit -ne 0 -or -not(Test-Path -LiteralPath $disFile -PathType Leaf)){throw "spirv-dis failed for $($m.role)/$($m.key)"}
}
$rows=@($modules|ForEach-Object {[ordered]@{role=$_.role;semanticKey=$_.key;sha256=$_.sha;words=((Get-Item -LiteralPath $_.spv).Length/4);val='PASS';dis='PASS'}})
$receipt=[ordered]@{schema=1;status='PASS';apkBytesRehashed=$false;modules=$rows;toolDirectory=$output;externalDisassemblyDirectory=$disassembly;disassemblyArchivePolicy='excluded'}
$receipt|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $output 'curation-spv-check-receipt.json') -Encoding utf8
Write-Output "Independent SPIR-V validation/disassembly PASS for $($modules.Count) exact module payloads. Rerun logs: $output. Full disassemblies remain external and will not be curated."
