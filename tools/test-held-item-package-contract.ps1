[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$AndroidApkPath,
    [string]$WindowsZipPath = ""
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression

$requiredAttributionMarkers = @(
    "Production Gothic arming sword created with Meshy; runtime processing by Samfa12/Codex.",
    "Production medieval hand torch created with Meshy; runtime processing by Samfa12/Codex.",
    "01a03b99-8999-7adc-8590-536691aacb87",
    "01a03b9c-d300-78bb-adc2-8fc93a65306f",
    "01a03b9d-02b1-7b7b-86ee-5e8c1f47af51",
    "01a03ba0-23a8-7bd4-92bf-3f0493705c61",
    "CC BY 4.0"
)

function Test-HeldItemPackage {
    param(
        [Parameter(Mandatory = $true)][string]$PackagePath,
        [Parameter(Mandatory = $true)][string]$LicenceEntry,
        [Parameter(Mandatory = $true)][string]$AssetPrefix
    )

    $resolved = (Resolve-Path -LiteralPath $PackagePath).Path
    $archive = [IO.Compression.ZipFile]::OpenRead($resolved)
    try {
        $entries = @($archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
        $requiredEntries = @(
            "$AssetPrefix/models/weapons/runtime/asset.manifest.json",
            "$AssetPrefix/models/weapons/runtime/gothic-arming-sword-rh-lod0.runtime.glb",
            "$AssetPrefix/models/props/runtime/asset.manifest.json",
            "$AssetPrefix/models/props/runtime/gothic-hand-torch-lod0.runtime.glb",
            "$AssetPrefix/textures/held-items/runtime/asset.manifest.json",
            $LicenceEntry
        )
        foreach ($required in $requiredEntries) {
            if ($entries -cnotcontains $required) {
                throw "Held-item package contract missing exact entry '$required' in $resolved"
            }
        }
        $forbidden = @($entries | Where-Object {
            $_ -match '(^|/)(source|high)(/|$)' -or
            $_ -match '\.blend$' -or
            $_ -match '\.glb\.processing\.json$'
        })
        if ($forbidden.Count -ne 0) {
            throw "Held-item package contract found source/high entries in ${resolved}: $($forbidden -join ', ')"
        }

        $licence = $archive.GetEntry($LicenceEntry)
        if ($null -eq $licence) {
            throw "Held-item package contract could not open '$LicenceEntry' in $resolved"
        }
        $reader = [IO.StreamReader]::new($licence.Open())
        try { $licenceText = $reader.ReadToEnd() } finally { $reader.Dispose() }
        foreach ($marker in $requiredAttributionMarkers) {
            if (-not $licenceText.Contains($marker, [StringComparison]::Ordinal)) {
                throw "Held-item package attribution missing exact marker '$marker' in $resolved"
            }
        }
    } finally {
        $archive.Dispose()
    }
}

Test-HeldItemPackage -PackagePath $AndroidApkPath -LicenceEntry "assets/ASSET_LICENSES.md" -AssetPrefix "assets"
if (-not [string]::IsNullOrWhiteSpace($WindowsZipPath)) {
    Test-HeldItemPackage -PackagePath $WindowsZipPath -LicenceEntry "ASSET_LICENSES.md" -AssetPrefix "assets"
}

Write-Output "Held-item runtime package entries and exact sword/torch CC BY 4.0 attribution passed."
