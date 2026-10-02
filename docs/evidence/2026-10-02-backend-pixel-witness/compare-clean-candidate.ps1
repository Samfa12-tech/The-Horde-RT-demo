param([Parameter(Mandatory)][string]$CandidateRoot,
      [Parameter(Mandatory)][string]$ControlPipelineDirectory,
      [string]$ControlComputeDirectory,
      [Parameter(Mandatory)][string]$OutputPath,[switch]$HighFixtures,[switch]$ShippingParity,[switch]$EdgeWitness,
      [string]$ControlSourceCommit)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$controlCommit = 'd9be81e77493d7e5c9af01604f865d0d3ef11e48'
if($EdgeWitness) {
    if(-not $HighFixtures -or $ShippingParity){throw 'Edge witness is High Diagnostic only.'}
    $controlCommit='b041ea832b0b7fc1882bbf4239c695cc70fbcd45'
}
if(-not [string]::IsNullOrWhiteSpace($ControlSourceCommit)) {
    $controlCommit=(& git -C $repoRoot rev-parse --verify "${ControlSourceCommit}^{commit}").Trim()
    if($LASTEXITCODE -ne 0){throw 'Explicit retained-control source must resolve to a commit.'}
}
$expected = @('opening','skeleton','worst-bend','lantern-drop','skylight','yellow',
              'blue','red','green','mirror','lich','finale-roof','two-enemy-combat')
$fixtureIds = @{'glass-edge-fresnel'=113;'lantern-held-high'=116;'lantern-held-low'=117}
$quality = if($HighFixtures){'High'}else{'Mobile'}
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing.Common,System.Drawing.Primitives,System.Private.Windows.GdiPlus,System.Private.Windows.Core,System.Collections -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
public static class HordeCleanCandidatePixels {
    public static object Read(string left, string right, int skipRows=0) {
        using(var a=new Bitmap(left)) using(var b=new Bitmap(right)) {
            if(a.Width!=960 || a.Height!=540 || a.Size!=b.Size)
                throw new InvalidOperationException("Unexpected finite capture extent.");
            int maximum=0,count=0,outlierCount=0;
            var outliers=new List<object>();
            for(int y=skipRows;y<a.Height;y++) for(int x=0;x<a.Width;x++) {
                var p=a.GetPixel(x,y); var q=b.GetPixel(x,y);
                int d=Math.Max(Math.Abs(p.R-q.R),Math.Max(Math.Abs(p.G-q.G),Math.Abs(p.B-q.B)));
                maximum=Math.Max(maximum,d); if(d>1)count++;
                if(d>3) {
                    outlierCount++;
                    if(outliers.Count<32) outliers.Add(new {x,y,maximumChannelDifference=d,
                        leftRgb=new[]{(int)p.R,(int)p.G,(int)p.B},
                        rightRgb=new[]{(int)q.R,(int)q.G,(int)q.B}});
                }
            }
            double fraction=(double)count/(960*(540-skipRows));
            return new {maximumChannelDifference=maximum,pixelsDifferentByMoreThanOne=count,
                differentFraction=fraction,unchangedGate=maximum<=3 && fraction<=.001,
                outlierCount,firstOutliers=outliers};
        }
    }
}
'@
function Require([bool]$Condition,[string]$Message) { if(-not $Condition){throw $Message} }
function Equal-Json($Left,$Right) { ($Left|ConvertTo-Json -Depth 14 -Compress) -ceq ($Right|ConvertTo-Json -Depth 14 -Compress) }
function Read-Run([string]$Directory,[string]$Backend,[bool]$Candidate,[string]$Instrumentation='Diagnostic') {
    $m=Get-Content (Join-Path $Directory 'capture-manifest.json') -Raw|ConvertFrom-Json
    Require ($m.schemaVersion -eq 1 -and $m.complete -and $null -eq $m.error -and
        $m.source -ceq 'rt-storage-image' -and $m.sceneOnly -and -not $m.overlaysIncluded -and
        $m.settlingFrames -eq 12 -and $m.fixedAnimationTimeSeconds -eq 0 -and
        $m.buildId -ceq '1.6.1' -and $m.playerMountProfile -ceq 'AnatomicalBody' -and
        $m.executionBackend -ceq $Backend -and $m.captures.Count -eq $expected.Count -and
        ($m.captures.checkpoint -join ',') -ceq ($expected -join ',')) "Run identity mismatch: $Directory"
    $catalogName=if($Backend -ceq 'RayTracingPipeline'){'raygen'}else{'rayquery'}
    $catalog=if($Candidate){Get-Content (Join-Path $repoRoot "tools/$catalogName-variant-catalog.json") -Raw|ConvertFrom-Json}
             else{((& git -C $repoRoot show "${controlCommit}:tools/$catalogName-variant-catalog.json")|Out-String)|ConvertFrom-Json}
    foreach($member in @('opaqueFast','genericDielectric')) {
        $selected=$m.selectedRtPipelineBundle.$member
        $row=@($catalog.variants|Where-Object key -CEQ $selected.key)
        Require ($row.Count -eq 1 -and $row[0].instrumentation -ceq $Instrumentation -and
            $row[0].quality -ceq $quality -and $row[0].spirvSha256 -ceq $selected.sha256) "Selected module mismatch: $Directory/$member"
    }
    for($i=0;$i -lt $expected.Count;$i++) {
        $p=$m.captures[$i]
        $expectedId = if($HighFixtures){$fixtureIds[$expected[$i]]}else{$i}
        Require ($p.id -eq $expectedId -and $p.width -eq 960 -and $p.height -eq 540 -and
            $p.honestlyPresentedRtFrame -and $p.pixelFormat -ceq 'RGBA8' -and
            $p.outputRedBlueSwapAppliedAndNormalised) "Capture presentation mismatch: $Directory/$i"
        Require ((Get-FileHash (Join-Path $Directory $p.file)).Hash.ToLowerInvariant() -ceq $p.pngSha256) "PNG hash mismatch: $Directory/$i"
        foreach($name in @('viewmodelGeometry','playerWorldBodyGeometry')) {
            $g=$p.$name
            Require ($g.available -and $g.space -ceq 'model' -and $g.source -ceq 'cpu-upload' -and
                (Get-FileHash (Join-Path $Directory $g.file)).Hash.ToLowerInvariant() -ceq $g.sha256) "CPU geometry mismatch: $Directory/$i/$name"
        }
    }
    [pscustomobject]@{directory=$Directory;manifest=$m}
}
function Compare-Runs($Left,$Right,[string]$Label) {
    $a=$Left.manifest; $b=$Right.manifest
    Require ((Equal-Json $a.device $b.device) -and (Equal-Json $a.presentation $b.presentation)) "Device/presentation mismatch: $Label"
    foreach($name in @('enabled','vertexBytes','indexBytes','materialBytes','instanceMetadataBytes',
        'primitiveMetadataBytes','textureBytes','descriptorCount','blasBytes','swordBlasBytes','torchBlasBytes','productionPropBlasBytes')) {
        Require ($a.staticRtAsset.$name -ceq $b.staticRtAsset.$name) "Static allocation mismatch: $Label/$name"
    }
    $rows=for($i=0;$i -lt $expected.Count;$i++) {
        $p=$a.captures[$i]; $q=$b.captures[$i]
        foreach($name in @('checkpoint','file','preset','zone','camera','requestedCamera','state',
            'viewmodelGeometry','playerWorldBodyGeometry')) {
            Require (Equal-Json $p.$name $q.$name) "Scene/provenance mismatch: $Label/$i/$name"
        }
        foreach($name in @('playerPrimaryVisible','playerWorldBodyInstanceFlags','instanceMasks','rewardGrip')) {
            Require (Equal-Json $p.visibility.$name $q.visibility.$name) "Visibility/grip mismatch: $Label/$i/$name"
        }
        [ordered]@{checkpoint=$p.checkpoint;file=$p.file;leftSha256=$p.pngSha256;rightSha256=$q.pngSha256;
            leftPrimaryPixels=$p.visibility.primaryPixels;rightPrimaryPixels=$q.visibility.primaryPixels;
            comparison=[HordeCleanCandidatePixels]::Read((Join-Path $Left.directory $p.file),(Join-Path $Right.directory $q.file),$(if($EdgeWitness){1}else{0}))}
    }
    [ordered]@{label=$Label;passed=(@($rows|Where-Object {-not $_.comparison.unchangedGate}).Count -eq 0);
        leftManifestSha256=(Get-FileHash (Join-Path $Left.directory 'capture-manifest.json')).Hash.ToLowerInvariant();
        rightManifestSha256=(Get-FileHash (Join-Path $Right.directory 'capture-manifest.json')).Hash.ToLowerInvariant();
        records=@($rows)}
}
if($ShippingParity) {
    Require (-not $HighFixtures) 'This finite instrumentation matrix is Mobile only.'
    Require (-not [string]::IsNullOrWhiteSpace($ControlComputeDirectory)) 'Shipping parity requires retained Diagnostic images on both backends.'
    $pipeline=Read-Run (Join-Path $CandidateRoot 'pipeline') 'RayTracingPipeline' $true 'Shipping'
    $compute=Read-Run (Join-Path $CandidateRoot 'compute') 'RayQueryCompute' $true 'Shipping'
    $diagnosticPipeline=Read-Run $ControlPipelineDirectory 'RayTracingPipeline' $true
    $diagnosticCompute=Read-Run $ControlComputeDirectory 'RayQueryCompute' $true
    $comparisons=@((Compare-Runs $diagnosticPipeline $pipeline 'shipping-diagnostic-parity/pipeline'),
        (Compare-Runs $diagnosticCompute $compute 'shipping-diagnostic-parity/compute'),
        (Compare-Runs $pipeline $compute 'shipping-backend-parity'))
} elseif($HighFixtures) {
    $comparisons=@(foreach($checkpoint in @('glass-edge-fresnel','lantern-held-high','lantern-held-low')) {
        if($EdgeWitness -and $checkpoint -cne 'glass-edge-fresnel'){continue}
        $expected=@($checkpoint)
        $pipeline=Read-Run (Join-Path $CandidateRoot "pipeline/$checkpoint") 'RayTracingPipeline' $true
        $compute=Read-Run (Join-Path $CandidateRoot "compute/$checkpoint") 'RayQueryCompute' $true
        $controlPipeline=Read-Run (Join-Path $ControlPipelineDirectory $checkpoint) 'RayTracingPipeline' $false
        Compare-Runs $pipeline $compute "candidate-backend-parity/$checkpoint"
        Compare-Runs $controlPipeline $pipeline "pipeline-old-control-differences/$checkpoint"
        if(-not [string]::IsNullOrWhiteSpace($ControlComputeDirectory)) {
            $controlCompute=Read-Run (Join-Path $ControlComputeDirectory "compute/$checkpoint") 'RayQueryCompute' $false
            Compare-Runs $controlPipeline $controlCompute "normal-control-backend-parity/$checkpoint"
            Compare-Runs $controlCompute $compute "compute-old-control-differences/$checkpoint"
        }
    })
} else {
    Require (-not [string]::IsNullOrWhiteSpace($ControlComputeDirectory)) 'Mobile comparison requires its retained compute control.'
    $pipeline=Read-Run (Join-Path $CandidateRoot 'pipeline') 'RayTracingPipeline' $true
    $compute=Read-Run (Join-Path $CandidateRoot 'compute') 'RayQueryCompute' $true
    $controlPipeline=Read-Run $ControlPipelineDirectory 'RayTracingPipeline' $false
    $controlCompute=Read-Run $ControlComputeDirectory 'RayQueryCompute' $false
    $comparisons=@((Compare-Runs $pipeline $compute 'candidate-backend-parity'),
        (Compare-Runs $controlPipeline $pipeline 'pipeline-old-control-differences'),
        (Compare-Runs $controlCompute $compute 'compute-old-control-differences'))
}
[ordered]@{schema=1;controlSource=$controlCommit;runReceipt=Get-Content (Join-Path $CandidateRoot 'run-receipt.json') -Raw|ConvertFrom-Json;
    payloadRows=$(if($EdgeWitness){1}else{0});pixelTolerance=[ordered]@{maximumChannelDifference=3;maximumFractionOverOne=0.001};
    performanceEvidence=$false;shippingDiagnosticMatrix=[bool]$ShippingParity;comparisons=$comparisons}|
    ConvertTo-Json -Depth 14|Set-Content -LiteralPath $OutputPath -Encoding utf8NoBOM
foreach($comparison in $comparisons) {
    foreach($row in $comparison.records) {
        '{0}/{1}: max={2}, fraction={3}, gate={4}, outliers={5}' -f $comparison.label,
            $row.checkpoint,$row.comparison.maximumChannelDifference,$row.comparison.differentFraction,
            $row.comparison.unchangedGate,$row.comparison.outlierCount
    }
}
# Old controls contain demonstrated rendering defects. Differences are retained,
# not automatically labelled regressions or forced back to those old pixels.
# ShippingParity exits on instrumentation equivalence only. Its independently
# labelled Shipping backend comparison may still fail and is retained in full.
if(@($comparisons|Where-Object {
    ($_.label.StartsWith('candidate-backend-parity') -or $_.label.StartsWith('shipping-diagnostic-parity')) -and -not $_.passed
}).Count -gt 0){
    throw "Clean backend image gate failed; retain $OutputPath"
}
