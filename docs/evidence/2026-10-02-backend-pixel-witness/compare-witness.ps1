param(
    [Parameter(Mandatory)][string]$ControlRoot,
    [Parameter(Mandatory)][string]$WitnessRoot,
    [Parameter(Mandatory)][string]$OutputPath
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing.Common,System.Drawing.Primitives,System.Private.Windows.GdiPlus,System.Private.Windows.Core -TypeDefinition @'
using System;
using System.Drawing;
public static class HordeBackendWitnessPixels {
    public sealed class Result {
        public int maximumChannelDifference;
        public int pixelsDifferentByMoreThanOne;
        public double differentFraction;
        public int[] controlRgb;
        public int[] witnessRgb;
        public float[] fields;
    }
    public static Result Read(string control, string witness, int x, int y, int row) {
        using (var a = new Bitmap(control)) using (var b = new Bitmap(witness)) {
            if (a.Width != 960 || a.Height != 540 || a.Size != b.Size)
                throw new InvalidOperationException("Unexpected witness extent.");
            var result = new Result();
            for (int py = 5; py < 540; ++py) for (int px = 0; px < 960; ++px) {
                Color p = a.GetPixel(px,py), q = b.GetPixel(px,py);
                int d = Math.Max(Math.Abs(p.R-q.R), Math.Max(Math.Abs(p.G-q.G), Math.Abs(p.B-q.B)));
                result.maximumChannelDifference = Math.Max(result.maximumChannelDifference,d);
                if (d > 1) ++result.pixelsDifferentByMoreThanOne;
            }
            result.differentFraction = (double)result.pixelsDifferentByMoreThanOne / (960*535);
            Color original = a.GetPixel(x,y), observed = b.GetPixel(x,y);
            result.controlRgb = new int[] {original.R,original.G,original.B};
            result.witnessRgb = new int[] {observed.R,observed.G,observed.B};
            result.fields = new float[48];
            for (int field = 0; field < 48; ++field) {
                Color p = b.GetPixel(field*2,row), q = b.GetPixel(field*2+1,row);
                if (p.A != 255 || q.A != 255 || q.G != 0 || q.B != 0)
                    throw new InvalidOperationException("Invalid lossless payload framing.");
                uint bits = (uint)p.R | ((uint)p.G<<8) | ((uint)p.B<<16) | ((uint)q.R<<24);
                float value = BitConverter.Int32BitsToSingle(unchecked((int)bits));
                if (!float.IsFinite(value)) throw new InvalidOperationException("Nonfinite witness field.");
                result.fields[field] = value;
            }
            if (result.fields[0] != 12345 || result.fields[1] != x || result.fields[2] != y)
                throw new InvalidOperationException("Witness coordinates/schema sentinel disagree.");
            return result;
        }
    }
}
'@
$names = @('sentinel','x','y','hit','instance','primitive','material','t',
    'geometricNormalX','geometricNormalY','geometricNormalZ','normalX','normalY','normalZ',
    'baseR','baseG','baseB','roughness','reflectivity','metallic','emissive','transmission',
    'materialFlags','rayDirectionX','rayDirectionY','rayDirectionZ','positionX','positionY','positionZ',
    'surfaceR','surfaceG','surfaceB','fireR','fireG','fireB','fireA',
    'afterFireR','afterFireG','afterFireB','afterMistR','afterMistG','afterMistB',
    'displayR','displayG','displayB','originX','originY','originZ')
$cases = @(
    @{id=2; file='02-worst-bend.png'; x=556; y=378; row=0},
    @{id=6; file='06-blue.png'; x=396; y=262; row=1},
    @{id=7; file='07-red.png'; x=396; y=262; row=1},
    @{id=11; file='11-finale-roof.png'; x=552; y=395; row=2},
    @{id=11; file='11-finale-roof.png'; x=770; y=526; row=3},
    @{id=12; file='12-two-enemy-combat.png'; x=564; y=393; row=4})
$rows = foreach ($backend in @('pipeline','compute')) {
    $oldDirectory = Join-Path $ControlRoot "control-windows-$backend"
    $newDirectory = Join-Path $WitnessRoot $backend
    $old = Get-Content (Join-Path $oldDirectory 'capture-manifest.json') -Raw | ConvertFrom-Json
    $new = Get-Content (Join-Path $newDirectory 'capture-manifest.json') -Raw | ConvertFrom-Json
    $expectedBackend = if ($backend -eq 'pipeline') {'RayTracingPipeline'} else {'RayQueryCompute'}
    if (-not $new.complete -or -not $new.investigationOnly -or $new.payloadRows -ne 5 -or
        $new.investigation -cne 'six-backend-pixel-output-witness' -or
        $new.captures.Count -ne 5 -or $new.settlingFrames -ne 12 -or
        $new.executionBackend -cne $expectedBackend -or
        ($new.captures.id -join ',') -cne '2,6,7,11,12' -or
        $new.presentation.dispatchWidth -ne 960 -or $new.presentation.dispatchHeight -ne 540 -or
        $new.device.gpuName -cne $old.device.gpuName -or
        ($new.staticRtAsset | ConvertTo-Json -Depth 5 -Compress) -cne
        ($old.staticRtAsset | ConvertTo-Json -Depth 5 -Compress)) {
        throw "Invalid witness scene/backend identity: $backend"
    }
    foreach ($case in $cases) {
        $oldCapture = @($old.captures | Where-Object file -CEQ $case.file)
        $newCapture = @($new.captures | Where-Object file -CEQ $case.file)
        if ($oldCapture.Count -ne 1 -or $newCapture.Count -ne 1) { throw 'Capture roster mismatch.' }
        $a = $oldCapture[0]; $b = $newCapture[0]
        if (-not $b.honestlyPresentedRtFrame -or
            ($a.camera | ConvertTo-Json -Compress) -cne ($b.camera | ConvertTo-Json -Compress) -or
            ($a.state | ConvertTo-Json -Compress) -cne ($b.state | ConvertTo-Json -Compress) -or
            $a.viewmodelGeometry.sha256 -cne $b.viewmodelGeometry.sha256 -or
            $a.playerWorldBodyGeometry.sha256 -cne $b.playerWorldBodyGeometry.sha256) {
            throw "Camera/state/CPU geometry differs: $backend/$($case.file)"
        }
        $oldPng = Join-Path $oldDirectory $case.file
        $newPng = Join-Path $newDirectory $case.file
        if ((Get-FileHash $oldPng).Hash.ToLowerInvariant() -cne $a.pngSha256 -or
            (Get-FileHash $newPng).Hash.ToLowerInvariant() -cne $b.pngSha256) { throw 'PNG hash mismatch.' }
        $result = [HordeBackendWitnessPixels]::Read($oldPng,$newPng,$case.x,$case.y,$case.row)
        $fields = [ordered]@{}
        for ($index=0; $index -lt $names.Count; ++$index) { $fields[$names[$index]] = $result.fields[$index] }
        $pixelPreserved = ($result.controlRgb -join ',') -ceq ($result.witnessRgb -join ',')
        $primaryCountsPreserved = ($a.visibility.primaryPixels | ConvertTo-Json -Compress) -ceq
            ($b.visibility.primaryPixels | ConvertTo-Json -Compress)
        [ordered]@{
            backend=$backend; file=$case.file; x=$case.x; y=$case.y
            controlSha256=$a.pngSha256; witnessSha256=$b.pngSha256
            controlRgb=$result.controlRgb; witnessRgb=$result.witnessRgb
            sourcePixelPreserved=$pixelPreserved; primaryCountsPreserved=$primaryCountsPreserved
            maximumChannelDifferenceOutsidePayload=$result.maximumChannelDifference
            pixelsDifferentByMoreThanOneOutsidePayload=$result.pixelsDifferentByMoreThanOne
            differentFractionOutsidePayload=$result.differentFraction
            unchangedImageGate=($result.maximumChannelDifference -le 3 -and $result.differentFraction -le 0.001)
            fields=$fields
        }
    }
}
[ordered]@{
    schema=1; investigationOnly=$true; payloadRows=5; fields=$names; records=@($rows)
    validity='Output probe is interpretable at an original outlier only when source pixel and meaningful scene facts recur; a probe is not a corrected-image acceptance.'
} | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $OutputPath -Encoding utf8NoBOM
$rows | ForEach-Object {
    '{0}/{1} ({2},{3}) preserved={4} primary={5}/{6}/{7} surface=({8},{9},{10}) gate={11}' -f
        $_.backend,$_.file,$_.x,$_.y,$_.sourcePixelPreserved,$_.fields.instance,
        $_.fields.primitive,$_.fields.material,$_.fields.surfaceR,$_.fields.surfaceG,
        $_.fields.surfaceB,$_.unchangedImageGate
}
