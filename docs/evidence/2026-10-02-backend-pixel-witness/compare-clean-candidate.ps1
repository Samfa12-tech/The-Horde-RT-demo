param([Parameter(Mandatory)][string]$CandidateRoot,
      [Parameter(Mandatory)][string]$ControlPipelineDirectory,
      [Parameter(Mandatory)][string]$ControlComputeDirectory,
      [Parameter(Mandatory)][string]$OutputPath)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$controlCommit = 'd9be81e77493d7e5c9af01604f865d0d3ef11e48'
$expected = @('opening','skeleton','worst-bend','lantern-drop','skylight','yellow',
              'blue','red','green','mirror','lich','finale-roof','two-enemy-combat')
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing.Common,System.Drawing.Primitives,System.Private.Windows.GdiPlus,System.Private.Windows.Core,System.Collections -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
public static class HordeCleanCandidatePixels {
    public static object Read(string left, string right) {
        using(var a=new Bitmap(left)) using(var b=new Bitmap(right)) {
            if(a.Width!=960 || a.Height!=540 || a.Size!=b.Size)
                throw new InvalidOperationException("Unexpected finite capture extent.");
            int maximum=0,count=0,outlierCount=0;
            var outliers=new List<object>();
            for(int y=0;y<a.Height;y++) for(int x=0;x<a.Width;x++) {
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
            double fraction=(double)count/(960*540);
            return new {maximumChannelDifference=maximum,pixelsDifferentByMoreThanOne=count,
                differentFraction=fraction,unchangedGate=maximum<=3 && fraction<=.001,
                outlierCount,firstOutliers=outliers};
        }
    }
}
'@
function Require([bool]$Condition,[string]$Message) { if(-not $Condition){throw $Message} }
function Equal-Json($Left,$Right) { ($Left|ConvertTo-Json -Depth 14 -Compress) -ceq ($Right|ConvertTo-Json -Depth 14 -Compress) }
function Read-Run([string]$Directory,[string]$Backend,[bool]$Candidate) {
    $m=Get-Content (Join-Path $Directory 'capture-manifest.json') -Raw|ConvertFrom-Json
    Require ($m.schemaVersion -eq 1 -and $m.complete -and $null -eq $m.error -and
        $m.source -ceq 'rt-storage-image' -and $m.sceneOnly -and -not $m.overlaysIncluded -and
        $m.settlingFrames -eq 12 -and $m.fixedAnimationTimeSeconds -eq 0 -and
        $m.buildId -ceq '1.6.1' -and $m.playerMountProfile -ceq 'AnatomicalBody' -and
        $m.executionBackend -ceq $Backend -and $m.captures.Count -eq 13 -and
        ($m.captures.checkpoint -join ',') -ceq ($expected -join ',')) "Run identity mismatch: $Directory"
    $catalogName=if($Backend -ceq 'RayTracingPipeline'){'raygen'}else{'rayquery'}
    $catalog=if($Candidate){Get-Content (Join-Path $repoRoot "tools/$catalogName-variant-catalog.json") -Raw|ConvertFrom-Json}
             else{((& git -C $repoRoot show "${controlCommit}:tools/$catalogName-variant-catalog.json")|Out-String)|ConvertFrom-Json}
    foreach($member in @('opaqueFast','genericDielectric')) {
        $selected=$m.selectedRtPipelineBundle.$member
        $row=@($catalog.variants|Where-Object key -CEQ $selected.key)
        Require ($row.Count -eq 1 -and $row[0].instrumentation -ceq 'Diagnostic' -and
            $row[0].quality -ceq 'Mobile' -and $row[0].spirvSha256 -ceq $selected.sha256) "Selected module mismatch: $Directory/$member"
    }
    for($i=0;$i -lt 13;$i++) {
        $p=$m.captures[$i]
        Require ($p.id -eq $i -and $p.width -eq 960 -and $p.height -eq 540 -and
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
    $rows=for($i=0;$i -lt 13;$i++) {
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
            comparison=[HordeCleanCandidatePixels]::Read((Join-Path $Left.directory $p.file),(Join-Path $Right.directory $q.file))}
    }
    [ordered]@{label=$Label;passed=(@($rows|Where-Object {-not $_.comparison.unchangedGate}).Count -eq 0);
        leftManifestSha256=(Get-FileHash (Join-Path $Left.directory 'capture-manifest.json')).Hash.ToLowerInvariant();
        rightManifestSha256=(Get-FileHash (Join-Path $Right.directory 'capture-manifest.json')).Hash.ToLowerInvariant();
        records=@($rows)}
}
$pipeline=Read-Run (Join-Path $CandidateRoot 'pipeline') 'RayTracingPipeline' $true
$compute=Read-Run (Join-Path $CandidateRoot 'compute') 'RayQueryCompute' $true
$controlPipeline=Read-Run $ControlPipelineDirectory 'RayTracingPipeline' $false
$controlCompute=Read-Run $ControlComputeDirectory 'RayQueryCompute' $false
$comparisons=@((Compare-Runs $pipeline $compute 'candidate-backend-parity'),
    (Compare-Runs $controlPipeline $pipeline 'pipeline-old-control-differences'),
    (Compare-Runs $controlCompute $compute 'compute-old-control-differences'))
[ordered]@{schema=1;controlSource=$controlCommit;runReceipt=Get-Content (Join-Path $CandidateRoot 'run-receipt.json') -Raw|ConvertFrom-Json;
    payloadRows=0;pixelTolerance=[ordered]@{maximumChannelDifference=3;maximumFractionOverOne=0.001};
    performanceEvidence=$false;comparisons=$comparisons}|
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
if(-not $comparisons[0].passed){throw "Clean backend image gate failed; retain $OutputPath"}
