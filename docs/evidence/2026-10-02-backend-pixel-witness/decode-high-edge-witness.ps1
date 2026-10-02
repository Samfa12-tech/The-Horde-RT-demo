param([Parameter(Mandatory)][string]$CaptureRoot, [Parameter(Mandatory)][string]$OutputPath)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
function Field($Image, [int]$Index) {
    $a = $Image.GetPixel($Index * 2, 0)
    $b = $Image.GetPixel($Index * 2 + 1, 0)
    [BitConverter]::ToSingle([byte[]]@($a.R, $a.G, $a.B, $b.R), 0)
}
function Vector($Image, [int]$Index) { @(0..2 | ForEach-Object {Field $Image ($Index + $_)}) }
function ContactCandidate($Image, [int]$Index) {
    [ordered]@{hit=Field $Image $Index; instance=Field $Image ($Index+1);
        primitive=Field $Image ($Index+2); material=Field $Image ($Index+3);
        rawT=Field $Image ($Index+4); geometry=Field $Image ($Index+5);
        frontFace=Field $Image ($Index+6); flags=Field $Image ($Index+7);
        bary=@(Field $Image ($Index+8); Field $Image ($Index+9));
        transmission=Field $Image ($Index+10); signedAlignment=Field $Image ($Index+11);
        surfacePosition=Vector $Image ($Index+12); geometricNormal=Vector $Image ($Index+16)}
}
$records = foreach ($backend in @('pipeline', 'compute')) {
    $root = Join-Path $CaptureRoot "$backend/glass-edge-fresnel"
    $manifest = Get-Content (Join-Path $root 'capture-manifest.json') -Raw | ConvertFrom-Json
    if(-not $manifest.complete -or $manifest.captures.Count -ne 1) { throw 'Incomplete finite capture.' }
    $capture = $manifest.captures[0]
    $path = Join-Path $root $capture.file
    if ((Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant() -cne $capture.pngSha256) { throw 'PNG identity mismatch.' }
    $image = [Drawing.Bitmap]::new($path)
    try {
        if ($image.Width -ne 960 -or $image.Height -ne 540 -or (Field $image 0) -ne 12345 -or
            (Field $image 1) -ne 456 -or (Field $image 2) -ne 304) { throw 'Observer payload identity mismatch.' }
        $count = [int](Field $image 53)
        if ($count -lt 1 -or $count -gt 9) { throw 'Pixel did not enter the bounded production dielectric route.' }
        $pixel = $image.GetPixel(456,304)
        [ordered]@{
            backend=$backend; imageSha256=$capture.pngSha256; actualRgb=@($pixel.R,$pixel.G,$pixel.B)
            primary=[ordered]@{hit=Field $image 3; instance=Field $image 4; primitive=Field $image 5;
                material=Field $image 6; t=Field $image 7; geometricNormal=Vector $image 8;
                normal=Vector $image 11; base=Vector $image 14; roughness=Field $image 17;
                transmission=Field $image 18; flags=Field $image 19; direction=Vector $image 20;
                origin=Vector $image 23; position=Vector $image 26}
            surface=Vector $image 29; display=Vector $image 32; firstFresnel=Field $image 35
            reflection=[ordered]@{direction=Vector $image 36; origin=Vector $image 39; hit=Field $image 42;
                instance=Field $image 43; primitive=Field $image 44; material=Field $image 45;
                accumulatedDistance=Field $image 46; position=Vector $image 47; radiance=Vector $image 50}
            transmitted=Vector $image 54; throughput=Vector $image 57
            termination=[ordered]@{resolved=Field $image 60; overflow=Field $image 61;
                volumeOpen=Field $image 62; tirSinceTransition=Field $image 63}
            # Absent in the retained first observer: do not infer candidate
            # fields there. New availability probes are explicitly labelled.
            contactProbe=$(if ((Field $image 280) -eq 1) {
                [ordered]@{observed=$true; candidates=Field $image 281;
                    matchingExitCandidates=Field $image 282; opaqueCandidates=Field $image 283;
                    origin=Vector $image 284; minimum=Field $image 287;
                    direction=Vector $image 288; maximum=Field $image 291;
                    exit=ContactCandidate $image 292; receiver=ContactCandidate $image 312}
            } else { $null })
            worldPlaneMetadata=$(if ((Field $image 332) -eq 9876) {
                [ordered]@{coordinate=Field $image 333; flags=Field $image 334;
                    triangleCount=Field $image 335}
            } else { $null })
            contactPolicy=$(if ((Field $image 336) -eq 4321) {
                [ordered]@{consumed=Field $image 337; receiverPrimitive=Field $image 338;
                    coordinate=Field $image 339; flags=Field $image 340;
                    outgoing=Vector $image 341; gpuMathMask=Field $image 344}
            } else { $null })
            visits=@(for($visit=0;$visit -lt $count;++$visit) {
                $offset=64+24*$visit
                [ordered]@{visit=$visit; hit=Field $image $offset; instance=Field $image ($offset+1);
                    primitive=Field $image ($offset+2); material=Field $image ($offset+3); t=Field $image ($offset+4);
                    volumeOpen=Field $image ($offset+5); roughness=Field $image ($offset+6); ior=Field $image ($offset+7);
                    position=Vector $image ($offset+8); transmission=Field $image ($offset+11);
                    geometricNormal=Vector $image ($offset+12); signedAlignment=Field $image ($offset+15);
                    direction=Vector $image ($offset+16); previousSpawnEpsilon=Field $image ($offset+19);
                    queryOrigin=Vector $image ($offset+20); queryMinimum=Field $image ($offset+23)}
            })
        }
    } finally { $image.Dispose() }
}
[ordered]@{schema=1; pixel=@(456,304); reservedRows=@(0); records=@($records)} |
    ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $OutputPath -Encoding utf8NoBOM
Write-Output "Decoded bounded actual-call payload: $OutputPath"
