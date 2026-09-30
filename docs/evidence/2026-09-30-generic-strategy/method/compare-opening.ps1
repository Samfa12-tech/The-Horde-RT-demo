param([Parameter(Mandatory=$true)][string]$ControlDirectory,
      [Parameter(Mandatory=$true)][string]$ProfileDirectory)
$ErrorActionPreference = 'Stop'
$taskRoot = 'C:/Dev/tmp/horde-generic-route-profile-20260930'
$expectedApks = @('389d6954f7a3fddde1ced690470029fa4b14cc6f825fa34bf7269dc617f946e0',
    '6ec1d66cad09eb50a3ce9340a2d2031e259f477f13a949be0e4ce98cdc6f1d96')
$directories = @($ControlDirectory,$ProfileDirectory)
$states = @(); $pngs = @(); $hashes = @()
for ($i=0; $i -lt 2; ++$i) {
    $manifest = Get-Content -LiteralPath (Join-Path $directories[$i] 'capture-manifest.json') -Raw | ConvertFrom-Json
    $summary = Get-Content -LiteralPath (Join-Path $directories[$i] 'summary.json') -Raw | ConvertFrom-Json
    if ($manifest.device.model -cne 'SM-S948B' -or $manifest.scale -ne 75 -or
        $manifest.package -cne 'com.samfa12.hordelanternrt.debug' -or
        $manifest.installedApkSha256 -cne $expectedApks[$i] -or $manifest.apkSha256 -cne $expectedApks[$i] -or
        $manifest.checkpoints.Count -ne 1 -or $summary.failures.Count -ne 0 -or
        -not $manifest.lifecycle.homeResumePassed -or -not $manifest.lifecycle.honestPresentationAfterResume) { throw 'Invalid exact capture/lifecycle identity' }
    $capture = $manifest.checkpoints[0]
    $state = Get-Content -LiteralPath (Join-Path $directories[$i] $capture.nativeStateFile) -Raw | ConvertFrom-Json
    $frame = $state.rtFrameEvidence.completedFrame
    $strategy = @('opaque-fast','generic-dielectric')[$i]
    if ($capture.checkpoint -cne 'opening' -or $state.checkpoint -cne 'opening' -or
        $state.status -cne 'capture-ready' -or $state.captureStableFrames -ne 12 -or -not $state.presented -or
        $state.internalExtent.width -ne 1080 -or $state.internalExtent.height -ne 2235 -or
        $state.playerRenderRoute -cne 'modelled-viewmodel' -or -not $state.dedicatedPlayerPrimaryOwnership -or
        $state.executionBackend -cne 'RayTracingPipeline' -or
        $frame.pipeline.activeStrategy -cne $strategy -or $frame.pipeline.instrumentation -cne 'diagnostic' -or
        $frame.pipeline.dielectricQuality -cne 'mobile' -or $frame.presentation.outcome -cne 'presented' -or
        -not $frame.dispatch.rtDispatchRecorded -or -not $frame.dispatch.swapchainCopyRecorded -or
        $frame.dielectric.completedSubmissionSerial -ne $frame.identity.submissionSerial) { throw 'Invalid owning frame or strategy' }
    for ($counter=0; $counter -lt 35; ++$counter) {
        if ($frame.dielectric.counters[$counter] -ne 0) { throw "Unexpected opaque-scene dielectric counter $counter" }
    }
    $png = Join-Path $directories[$i] $capture.png.file
    $hash = (Get-FileHash -LiteralPath $png).Hash.ToLowerInvariant()
    if ($hash -cne $capture.png.sha256) { throw 'Capture PNG hash mismatch' }
    $states += $state; $pngs += $png; $hashes += $hash
}
foreach ($field in @('player','playerCombat','zone','renderScale','waterQuality','rtLab','internalExtent','swapchainExtent',
        'playerRenderRoute','playerMountProfile','dedicatedPlayerPrimaryOwnership','animationTime','torchFailurePhase','lich','tlasInstanceCount')) {
    $a = $states[0].$field | ConvertTo-Json -Depth 20 -Compress
    $b = $states[1].$field | ConvertTo-Json -Depth 20 -Compress
    if ($a -cne $b) { throw "Authored scene differs: $field" }
}
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies @('System.Drawing.Common', 'System.Drawing.Primitives', 'System.Private.Windows.GdiPlus', 'System.Private.Windows.Core') -TypeDefinition @'
using System;
using System.Drawing;
public static class HordeOpeningPixelComparison {
    public static long[] Compare(string before, string after) {
        using (var a = new Bitmap(before)) using (var b = new Bitmap(after)) {
            if (a.Width != b.Width || a.Height != b.Height) throw new Exception("Different PNG extents");
            long different=0, overOne=0, maximum=0, sum=0;
            for (int y=0;y<a.Height;++y) for(int x=0;x<a.Width;++x) {
                var p=a.GetPixel(x,y); var q=b.GetPixel(x,y);
                int r=Math.Abs(p.R-q.R),g=Math.Abs(p.G-q.G),bl=Math.Abs(p.B-q.B);
                int d=Math.Max(r,Math.Max(g,bl));
                if(d>0) ++different; if(d>1) ++overOne; maximum=Math.Max(maximum,d); sum+=r+g+bl;
            }
            return new long[]{(long)a.Width*a.Height,different,overOne,maximum,sum,a.Width,a.Height};
        }
    }
}
'@
$pixels = [HordeOpeningPixelComparison]::Compare($pngs[0],$pngs[1])
$result = [ordered]@{
    schema=1;scope='Same exact-phone opaque opening Diagnostic/Mobile subset, whole-strategy investigation; not Shipping/Diagnostic parity or full backend/visual acceptance'
    deviceModel='SM-S948B';scale=75;backend='RayTracingPipeline';source='Control reused exact389d6954; profile eafbf826 plus force-generic patch, not clean engineering HEAD'
    exactApkSha256=$expectedApks;pngSha256=$hashes;authoredSceneEqual=$true;owningStrategies=@('opaque-fast','generic-dielectric')
    owningFrameIdentities=@($states[0].rtFrameEvidence.completedFrame.identity,$states[1].rtFrameEvidence.completedFrame.identity)
    toleranceSource='tools/compare-foundation-captures.ps1, pixel-only limits unchanged'
    tolerance=[ordered]@{maximumChannelDifference=3;maximumFractionOverOne=0.001}
    pixels=$pixels[0];differentPixels=$pixels[1];pixelsDifferentByMoreThanOne=$pixels[2];maximumChannelDifference=$pixels[3]
    fractionOverOne=$pixels[2]/$pixels[0];meanAbsoluteRgbDifference=$pixels[4]/(3.0*$pixels[0]);width=$pixels[5];height=$pixels[6]
    pixelSubsetPassed=$pixels[3] -le 3 -and ($pixels[2]/$pixels[0]) -le 0.001
    optimisationAcceptance=$false;performanceClaim='none';phase4Complete=$false
}
$output="$taskRoot/opening-image-comparison.json"
if(Test-Path -LiteralPath $output){throw 'Do not overwrite frozen image receipt'}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $output -Encoding utf8NoBOM
$result | ConvertTo-Json -Depth 8
