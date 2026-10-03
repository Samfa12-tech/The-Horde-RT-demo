$ErrorActionPreference = 'Stop'
$root = 'C:/Dev/tmp/horde-glass-isolated-20261001'
$value = Get-Content (Join-Path $root 'marker-analysis.json') -Raw | ConvertFrom-Json
$targets = @($value.markerCoordinates.interfaceBudget) + @($value.markerCoordinates.certifiedBudgetReason2)
if ($targets.Count -ne 81 -or ($targets | ForEach-Object { $_ -join ',' } | Sort-Object -Unique).Count -ne 81) {
    throw 'Expected exactly 81 independently located native marker coordinates.'
}
$lines = @('// Generated from the current native marker capture, not historical coordinates.',
    '#if HORDE_LOCAL_ISOLATED_GLASS_MARKER',
    'const int kIsolatedTargetCount = 81;',
    'const uvec2 kIsolatedTargets[81] = uvec2[81](')
for ($i = 0; $i -lt $targets.Count; ++$i) {
    $suffix = if ($i -eq $targets.Count - 1) { ');' } else { ',' }
    $lines += '    uvec2(' + $targets[$i][0] + 'u, ' + $targets[$i][1] + 'u)' + $suffix
}
$lines += '#endif'
[IO.File]::WriteAllText((Join-Path $root 'source/shaders/raytracing/include/rt_isolated_glass_targets.generated.glsl'),
    (($lines -join "`n") + "`n"), [Text.UTF8Encoding]::new($false))
Write-Output 'Generated exactly 81 current-image targets.'
