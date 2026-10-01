param(
    [Parameter(Mandatory)][string]$ControlRoot,
    [Parameter(Mandatory)][string]$CurrentRoot,
    [Parameter(Mandatory)][string]$OutputPath
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing.Common,System.Drawing.Primitives,System.Private.Windows.GdiPlus,System.Private.Windows.Core -TypeDefinition @'
using System;
using System.Drawing;
public static class HordeRow43Pixels {
    public sealed class Result {
        public int maximumChannelDifference;
        public int pixelsDifferentByMoreThanOne;
        public double differentFraction;
        public int[] controlWitnessRgb;
        public int[] currentWitnessRgb;
    }
    public static Result Compare(string control, string current) {
        using (var a = new Bitmap(control)) using (var b = new Bitmap(current)) {
            if (a.Width != 1232 || a.Height != 803 || a.Size != b.Size)
                throw new InvalidOperationException("Unexpected row43 extent.");
            var result = new Result();
            for (int y = 0; y < a.Height; ++y) for (int x = 0; x < a.Width; ++x) {
                Color p = a.GetPixel(x,y), q = b.GetPixel(x,y);
                int d = Math.Max(Math.Abs(p.R-q.R), Math.Max(Math.Abs(p.G-q.G), Math.Abs(p.B-q.B)));
                result.maximumChannelDifference = Math.Max(result.maximumChannelDifference,d);
                if (d > 1) ++result.pixelsDifferentByMoreThanOne;
                if (x == 524 && y == 552) {
                    result.controlWitnessRgb = new int[] {p.R,p.G,p.B};
                    result.currentWitnessRgb = new int[] {q.R,q.G,q.B};
                }
            }
            result.differentFraction = (double)result.pixelsDifferentByMoreThanOne / (a.Width*a.Height);
            return result;
        }
    }
}
'@
$rows = foreach ($backend in @('pipeline','compute')) {
    $oldDirectory = Join-Path $ControlRoot $backend
    $newDirectory = Join-Path $CurrentRoot $backend
    $old = Get-Content (Join-Path $oldDirectory 'capture-manifest.json') -Raw | ConvertFrom-Json
    $new = Get-Content (Join-Path $newDirectory 'capture-manifest.json') -Raw | ConvertFrom-Json
    if (-not $new.complete -or -not $new.investigationOnly -or
        $new.liveSimulationTick -ne 646 -or $new.liveBenchmarkFrame -ne 44 -or
        $new.captures.Count -ne 1 -or -not $new.captures[0].honestlyPresentedRtFrame) {
        throw "Invalid replay identity: $backend"
    }
    $counterCount = 0
    foreach ($group in @('dielectricDiagnostics','dielectricReasonDiagnostics')) {
        if (-not $new.$group.available) { throw "Counters unavailable: $backend" }
        foreach ($field in $old.$group.psobject.Properties) {
            if ($field.Name -in @('available','availability')) { continue }
            if ($field.Value -ne $new.$group.($field.Name)) { throw "Counter changed: $backend $($field.Name)" }
            ++$counterCount
        }
    }
    if ($counterCount -ne 41) { throw "Counter roster changed: $counterCount" }
    foreach ($geometry in @('viewmodelGeometry','playerWorldBodyGeometry')) {
        if ($old.captures[0].$geometry.sha256 -ne $new.captures[0].$geometry.sha256) {
            throw "CPU geometry changed: $backend $geometry"
        }
    }
    if (($old.selectedRtPipelineBundle | ConvertTo-Json -Compress) -ne
        ($new.selectedRtPipelineBundle | ConvertTo-Json -Compress)) { throw "Modules changed: $backend" }
    $oldImage = [IO.Path]::GetFullPath((Join-Path $oldDirectory $old.captures[0].file))
    $newImage = [IO.Path]::GetFullPath((Join-Path $newDirectory $new.captures[0].file))
    $oldPngHash = (Get-FileHash -LiteralPath $oldImage).Hash.ToLowerInvariant()
    $newPngHash = (Get-FileHash -LiteralPath $newImage).Hash.ToLowerInvariant()
    if ($oldPngHash -ne $old.captures[0].pngSha256 -or
        $newPngHash -ne $new.captures[0].pngSha256) {
        throw "PNG bytes do not match manifest: $backend (hydrate LFS pointers before comparison)."
    }
    $pixels = [HordeRow43Pixels]::Compare($oldImage,$newImage)
    [pscustomobject]@{
        backend = $new.executionBackend; counterCount = $counterCount
        matchedCpuGeometryAndModules = $true
        primaryMismatchedExitCount = $new.dielectricReasonDiagnostics.primaryMismatchedExitCount
        controlPngSha256 = $oldPngHash
        currentPngSha256 = $newPngHash
        pixels = $pixels
        pixelGatePassed = $pixels.maximumChannelDifference -le 3 -and $pixels.differentFraction -le 0.001
    }
}
[pscustomobject]@{ schema=1; sourceBase='a32a718'; pixelTolerance=@{maxRgb=3;maximumFractionOverOne=0.001}; rows=@($rows) } |
    ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutputPath -Encoding utf8
$rows | ConvertTo-Json -Depth 5
if (@($rows | Where-Object { -not $_.pixelGatePassed }).Count) { throw 'Current replay pixel gate failed; preserve the result.' }
