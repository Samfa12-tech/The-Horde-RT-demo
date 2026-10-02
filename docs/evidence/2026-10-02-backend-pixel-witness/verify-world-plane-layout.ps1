param([Parameter(Mandatory)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
if (Test-Path -LiteralPath $OutputRoot) { throw 'Preserve completed layout evidence; use a new output root.' }
if (-not [BitConverter]::IsLittleEndian) { throw 'Embedded-word extraction requires the recorded little-endian host.' }
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$rows = foreach ($catalogName in @('raygen-variant-catalog.json','rayquery-variant-catalog.json')) {
    $catalog = Get-Content (Join-Path $repoRoot "tools/$catalogName") -Raw | ConvertFrom-Json
    foreach ($variant in $catalog.variants) {
        $include = Join-Path $repoRoot $variant.artifactPath
        $matches = [regex]::Matches([IO.File]::ReadAllText($include), '0x([0-9a-fA-F]{8})u')
        $bytes = [byte[]]::new($matches.Count * 4)
        for ($index = 0; $index -lt $matches.Count; ++$index) {
            $word = [Convert]::ToUInt32($matches[$index].Groups[1].Value,16)
            [BitConverter]::GetBytes($word).CopyTo($bytes,$index * 4)
        }
        $spirv = Join-Path $OutputRoot "$($variant.key).spv"
        $assembly = Join-Path $OutputRoot "$($variant.key).spvasm"
        [IO.File]::WriteAllBytes($spirv,$bytes)
        if ($bytes.Length -ne $variant.bytes -or
            (Get-FileHash -LiteralPath $spirv).Hash.ToLowerInvariant() -cne $variant.spirvSha256) {
            throw "Actual embedded module disagrees with catalog: $($variant.key)"
        }
        & 'C:/VulkanSDK/1.4.350.0/Bin/spirv-val.exe' --target-env vulkan1.2 $spirv
        if ($LASTEXITCODE -ne 0) { throw "SPIR-V validation failed: $($variant.key)" }
        & 'C:/VulkanSDK/1.4.350.0/Bin/spirv-dis.exe' $spirv -o $assembly
        if ($LASTEXITCODE -ne 0) { throw "SPIR-V disassembly failed: $($variant.key)" }
        $text = [IO.File]::ReadAllText($assembly)
        $type = [regex]::Match($text,'OpName\s+(%\S+)\s+"RtWorldSurfaceGpu"').Groups[1].Value
        if (-not $type) { throw "No world-surface type: $($variant.key)" }
        for ($member = 0; $member -lt 3; ++$member) {
            if ($text -notmatch ('OpMemberDecorate\s+{0}\s+{1}\s+Offset\s+{2}\b' -f
                    [regex]::Escape($type),$member,($member * 4))) {
                throw "Wrong world-surface member offset: $($variant.key)/$member"
            }
        }
        $array = [regex]::Match($text, ('(%\S+)\s*=\s*OpTypeRuntimeArray\s+{0}\s' -f
            [regex]::Escape($type))).Groups[1].Value
        if (-not $array -or $text -notmatch ('OpDecorate\s+{0}\s+ArrayStride\s+12\b' -f [regex]::Escape($array)) -or
            $text -notmatch 'OpDecorate\s+%worldSurfaces\s+Binding\s+6\b' -or
            $text -notmatch 'OpDecorate\s+%worldSurfaces\s+DescriptorSet\s+0\b') {
            throw "Wrong world-surface array stride/binding: $($variant.key)"
        }
        $atomics = [regex]::Matches($text,'\bOpAtomic\w+\b').Count
        $diagnostics = $text -match 'OpDecorate\s+%\S+\s+Binding\s+22\b'
        $observer = $text -match '"investigationContactWorldPlane"'
        if ($variant.instrumentation -ceq 'Shipping' -and ($atomics -ne 0 -or $diagnostics -or $observer)) {
            throw "Shipping retains diagnostic/observer overhead: $($variant.key)"
        }
        [ordered]@{key=$variant.key; spirvSha256=$variant.spirvSha256; bytes=$bytes.Length;
            offsets=@(0,4,8); stride=12; binding=6; atomicInstructions=$atomics;
            diagnosticsBinding=$diagnostics; worldPlaneObserver=$observer; validated=$true}
    }
}
if (@($rows).Count -ne 16) { throw 'Expected exactly sixteen compiled modules.' }
[ordered]@{schema=1; sourceBase=(& git -C $repoRoot rev-parse HEAD).Trim();
    unadmitted=$true; modules=@($rows)} | ConvertTo-Json -Depth 6 |
    Set-Content (Join-Path $OutputRoot 'layout-receipt.json') -Encoding utf8NoBOM
Write-Output 'All16 actual embedded modules: offsets0/4/8, stride12, binding6, SPIR-V validation PASS; Shipping diagnostic overhead absent.'
