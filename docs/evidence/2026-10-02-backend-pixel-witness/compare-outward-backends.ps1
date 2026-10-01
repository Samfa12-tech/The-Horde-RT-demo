param([Parameter(Mandatory)][string]$CaptureRoot,[Parameter(Mandatory)][string]$OutputPath)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing.Common,System.Drawing.Primitives,System.Private.Windows.GdiPlus,System.Private.Windows.Core,System.Collections -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
public static class HordeOutwardBoxParity {
    public static object Read(string pipeline,string compute) {
        using(var a=new Bitmap(pipeline)) using(var b=new Bitmap(compute)) {
            if(a.Width!=960 || a.Height!=540 || a.Size!=b.Size)
                throw new InvalidOperationException("Unexpected finite capture extent.");
            int maximum=0,count=0;
            var outliers=new List<object>();
            for(int y=5;y<a.Height;y++) for(int x=0;x<a.Width;x++) {
                var p=a.GetPixel(x,y); var q=b.GetPixel(x,y);
                int d=Math.Max(Math.Abs(p.R-q.R),Math.Max(Math.Abs(p.G-q.G),Math.Abs(p.B-q.B)));
                maximum=Math.Max(maximum,d); if(d>1)count++;
                if(d>3)outliers.Add(new {x,y,maximumChannelDifference=d,
                    pipelineRgb=new[]{(int)p.R,(int)p.G,(int)p.B},
                    computeRgb=new[]{(int)q.R,(int)q.G,(int)q.B}});
            }
            double fraction=(double)count/(960*535);
            return new {maximumChannelDifference=maximum,pixelsDifferentByMoreThanOne=count,
                differentFraction=fraction,unchangedGate=maximum<=3 && fraction<=.001,outliers};
        }
    }
}
'@
$a=Get-Content (Join-Path $CaptureRoot 'pipeline/capture-manifest.json') -Raw|ConvertFrom-Json
$b=Get-Content (Join-Path $CaptureRoot 'compute/capture-manifest.json') -Raw|ConvertFrom-Json
if (-not $a.complete -or -not $b.complete -or $a.captures.Count -ne 5 -or
    $b.captures.Count -ne 5 -or $a.executionBackend -cne 'RayTracingPipeline' -or
    $b.executionBackend -cne 'RayQueryCompute' -or $a.settlingFrames -ne 12 -or
    $b.settlingFrames -ne 12 -or $a.device.gpuName -cne $b.device.gpuName) {
    throw 'Finite candidate identity/backend mismatch.'
}
$rows=foreach($p in $a.captures){
    $q=@($b.captures|Where-Object file -CEQ $p.file)
    if($q.Count -ne 1 -or -not $p.honestlyPresentedRtFrame -or -not $q[0].honestlyPresentedRtFrame -or
        ($p.camera|ConvertTo-Json -Compress) -cne ($q[0].camera|ConvertTo-Json -Compress) -or
        ($p.state|ConvertTo-Json -Compress) -cne ($q[0].state|ConvertTo-Json -Compress)) {throw 'Capture scene mismatch.'}
    $pipeline=Join-Path $CaptureRoot ('pipeline/'+$p.file)
    $compute=Join-Path $CaptureRoot ('compute/'+$p.file)
    if((Get-FileHash $pipeline).Hash.ToLowerInvariant() -cne $p.pngSha256 -or
        (Get-FileHash $compute).Hash.ToLowerInvariant() -cne $q[0].pngSha256) {throw 'Image hash mismatch.'}
    [ordered]@{file=$p.file;pipelineSha256=$p.pngSha256;computeSha256=$q[0].pngSha256;
        comparison=[HordeOutwardBoxParity]::Read($pipeline,$compute)}
}
[ordered]@{schema=1;candidate='7782e8eeb54850813c74c90b188969f908f4906f';payloadRows=5;
    toleranceMaximum=3;toleranceFraction=0.001;records=@($rows)}|ConvertTo-Json -Depth 9|
    Set-Content -LiteralPath $OutputPath -Encoding utf8NoBOM
$rows|ForEach-Object {'{0}: max={1}, fraction={2}, gate={3}, outliers={4}' -f
    $_.file,$_.comparison.maximumChannelDifference,$_.comparison.differentFraction,
    $_.comparison.unchangedGate,$_.comparison.outliers.Count}
