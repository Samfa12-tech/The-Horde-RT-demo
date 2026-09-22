$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$scriptPath = Join-Path $repoRoot 'tools/compile-raygen.ps1'
$tokens = $null
$errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw 'Raygen generator must parse before testing artifact ownership.' }
$function = $ast.Find({ param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
    $node.Name -eq 'Get-RaygenOwnedArtifactNames'
}, $true)
if ($null -eq $function) { throw 'Missing raygen artifact ownership function.' }
. ([scriptblock]::Create($function.Extent.Text))
$manifest = Get-Content (Join-Path $repoRoot 'tools/raygen-variants.json') -Raw | ConvertFrom-Json
$names = @($manifest.variants | ForEach-Object { "$($_.name).inc" } | Sort-Object)
if ($names.Count -ne 8) { throw 'Expected the eight authoritative raygen variants.' }
$temporaryBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$temporaryRoot = Join-Path $temporaryBase ('horde-raygen-ownership-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    foreach ($name in $names) {
        [IO.File]::WriteAllText((Join-Path $temporaryRoot $name), 'owned')
        [IO.File]::WriteAllText((Join-Path $temporaryRoot "rayquery_compute_$name"), 'other backend')
    }
    $actual = @(Get-RaygenOwnedArtifactNames $temporaryRoot $names)
    if (($actual -join '|') -cne ($names -join '|')) { throw 'Known compute artifacts polluted raygen ownership.' }
    $foreign = Join-Path $temporaryRoot 'rayquery_compute_unknown.inc'
    [IO.File]::WriteAllText($foreign, 'must not be hidden')
    $actual = @(Get-RaygenOwnedArtifactNames $temporaryRoot $names)
    if ($actual.Count -ne 9 -or $actual -cnotcontains 'rayquery_compute_unknown.inc') {
        throw 'Unknown compute-prefixed files must remain visible to strict directory validation.'
    }
    foreach ($name in $names) {
        if ([IO.File]::ReadAllText((Join-Path $temporaryRoot "rayquery_compute_$name")) -cne 'other backend') {
            throw 'Raygen ownership inspection changed a compute artifact.'
        }
    }
    Write-Output 'Raygen artifact ownership preserves the exact compute counterpart set and exposes unknown files.'
} finally {
    $resolved = [IO.Path]::GetFullPath($temporaryRoot)
    if ([IO.Directory]::GetParent($resolved).FullName -eq $temporaryBase.TrimEnd('\', '/') -and
        [IO.Path]::GetFileName($resolved).StartsWith('horde-raygen-ownership-')) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    } else { throw 'Refused unsafe fixture cleanup.' }
}
