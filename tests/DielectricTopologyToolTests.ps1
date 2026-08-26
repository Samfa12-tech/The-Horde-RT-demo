[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$validator = Join-Path $repoRoot "tools\validate-dielectric-topology.py"
$fixtureRoot = Join-Path $PSScriptRoot "fixtures\dielectric-topology"
$manifest = Join-Path $fixtureRoot "asset.manifest.json"

function Invoke-ExpectedValidation(
        [string]$name,
        [int]$expectedExitCode,
        [string]$expectedText,
        [string]$manifestName = "asset.manifest.json") {
    $asset = Join-Path $fixtureRoot $name
    $fixtureManifest = Join-Path $fixtureRoot $manifestName
    $savedErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    $output = & py -3 $validator $asset $fixtureManifest 2>&1 | Out-String
    $actualExitCode = $LASTEXITCODE
    $ErrorActionPreference = $savedErrorActionPreference
    if ($actualExitCode -ne $expectedExitCode) {
        throw "Topology validation for '$name' exited $actualExitCode instead of $expectedExitCode. Output: $output"
    }
    if ($output -notmatch [regex]::Escape($expectedText)) {
        throw "Topology validation for '$name' did not report '$expectedText'. Output: $output"
    }
}

Invoke-ExpectedValidation "closed-dielectric-lod0.runtime.glb" 0 "closed/manifold"
Invoke-ExpectedValidation "open-dielectric-lod0.runtime.glb" 2 "open thick dielectric volume"
Invoke-ExpectedValidation "non-manifold-dielectric-lod0.runtime.glb" 2 "referenced more than twice"
Invoke-ExpectedValidation "inward-dielectric-lod0.runtime.glb" 2 "inward-wound after baked node transforms"
Invoke-ExpectedValidation "single-face-flipped-dielectric-lod0.runtime.glb" 2 "used once in each direction"
Invoke-ExpectedValidation "negative-scale-dielectric-lod0.runtime.glb" 2 "negative-determinant transform"
Invoke-ExpectedValidation "valid-transformed-dielectric-lod0.runtime.glb" 0 "closed/manifold"
Invoke-ExpectedValidation "millimetre-dielectric-lod0.runtime.glb" 0 "closed/manifold"
Invoke-ExpectedValidation "transmission-only-lod0.runtime.glb" 0 "0 closed/manifold"
Invoke-ExpectedValidation "zero-thickness-lod0.runtime.glb" 0 "0 closed/manifold"
Invoke-ExpectedValidation "split-shell-dielectric-lod0.runtime.glb" 0 "1 closed/manifold thick component(s)"
Invoke-ExpectedValidation "disconnected-shells-dielectric-lod0.runtime.glb" 0 "2 closed/manifold thick component(s)"
Invoke-ExpectedValidation "mixed-orientation-shells-dielectric-lod0.runtime.glb" 2 "component 2"
Invoke-ExpectedValidation "split-flipped-face-dielectric-lod0.runtime.glb" 2 "component 1"
Invoke-ExpectedValidation "non-unit-min-seam-dielectric-lod0.runtime.glb" 0 "2 closed/manifold thick component(s)" "centimetre-units.manifest.json"
Invoke-ExpectedValidation "non-unit-min-seam-reordered-dielectric-lod0.runtime.glb" 0 "2 closed/manifold thick component(s)" "centimetre-units.manifest.json"
Invoke-ExpectedValidation "non-unit-min-scale-seam-dielectric-lod0.runtime.glb" 0 "2 closed/manifold thick component(s)" "centimetre-units.manifest.json"
Invoke-ExpectedValidation "non-unit-min-diagonal-dielectric-lod0.runtime.glb" 2 "component 2" "centimetre-units.manifest.json"
Invoke-ExpectedValidation "non-unit-max-disconnected-dielectric-lod0.runtime.glb" 0 "2 closed/manifold thick component(s)" "ten-metre-units.manifest.json"
Invoke-ExpectedValidation "non-unit-max-scale-seam-dielectric-lod0.runtime.glb" 2 "component 2" "ten-metre-units.manifest.json"
Invoke-ExpectedValidation "mixed-orientation-shells-reordered-dielectric-lod0.runtime.glb" 2 "component 2"
Invoke-ExpectedValidation "closed-dielectric-lod0.runtime.glb" 2 "metresPerUnit must be finite and greater than zero" "zero-units.manifest.json"
Invoke-ExpectedValidation "closed-dielectric-lod0.runtime.glb" 2 "metresPerUnit must be finite and greater than zero" "nonfinite-float-units.manifest.json"

$preparationSource = Get-Content -LiteralPath (Join-Path $repoRoot "tools\prepare-static-rt-asset.ps1") -Raw
if ($preparationSource -notmatch "validate-dielectric-topology\.py") {
    throw "Static RT preparation does not invoke the dielectric topology validator."
}

Write-Output "Dielectric offline topology validation contracts passed"
