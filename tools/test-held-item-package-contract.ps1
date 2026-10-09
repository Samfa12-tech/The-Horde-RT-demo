[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$AndroidApkPath,
    [string]$WindowsZipPath = "",
    [switch]$RequireHorde162World
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression

$requiredAttributionMarkers = @(
    "Production Gothic arming sword created with Meshy; runtime processing by Samfa12/Codex.",
    "Production medieval hand torch created with Meshy; runtime processing by Samfa12/Codex.",
    "Historical-Gothic traveller/fighter and viewmodel gauntlets created with Meshy; runtime processing and animation integration by Samfa12/Codex.",
    "Production Gothic reward chest created with Meshy; runtime processing by Samfa12/Codex.",
    "Production Gothic reward lantern created with Meshy; runtime processing by Samfa12/Codex.",
    "01a03b99-8999-7adc-8590-536691aacb87",
    "01a03b9c-d300-78bb-adc2-8fc93a65306f",
    "01a03b9d-02b1-7b7b-86ee-5e8c1f47af51",
    "01a03ba0-23a8-7bd4-92bf-3f0493705c61",
    "01a03c8c-1b40-733b-8a8b-0255f1384c22",
    "01a03c91-1472-765e-9a2f-d15370325975",
    "01a03c9d-5296-7994-8fe4-e68ecc77c7ac",
    "01a03ca2-a736-7808-b554-4f4193ec49f4",
    "01a03f69-2b15-7c6b-b17f-de4741cdafba",
    "01a03f6c-ba91-7f43-a42a-10d2f09f9f09",
    "01a03f74-72ac-7e62-8b58-cd906ddc758a",
    "01a03f69-327a-7c6d-ac8d-ccdbb9fe13d7",
    "01a03f6b-e99a-7ee2-9738-d6d236f89c2b",
    "01a03f70-0499-7047-976b-daef45aa7684",
    "01a03f74-7501-7164-8113-53ba917fc66d",
    "CC BY 4.0"
)
if ($RequireHorde162World) {
    $requiredAttributionMarkers += @('https://polyhaven.com/a/boulder_01', 'https://polyhaven.com/a/medieval_wall_02')
}

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
            "$AssetPrefix/models/props/runtime/dielectric-fixture/asset.manifest.json",
            "$AssetPrefix/models/props/runtime/dielectric-fixture/closed-glass-lod0.runtime.glb",
            "$AssetPrefix/models/props/runtime/gothic-chest-base/asset.manifest.json",
            "$AssetPrefix/models/props/runtime/gothic-chest-base/gothic-chest-base-lod0.runtime.glb",
            "$AssetPrefix/models/props/runtime/gothic-chest-lid/asset.manifest.json",
            "$AssetPrefix/models/props/runtime/gothic-chest-lid/gothic-chest-lid-lod0.runtime.glb",
            "$AssetPrefix/models/props/runtime/reward-lantern-ring/asset.manifest.json",
            "$AssetPrefix/models/props/runtime/reward-lantern-ring/reward-lantern-ring-lod0.runtime.glb",
            "$AssetPrefix/models/props/runtime/reward-lantern-body/asset.manifest.json",
            "$AssetPrefix/models/props/runtime/reward-lantern-body/reward-lantern-body-lod0.runtime.glb",
            "$AssetPrefix/models/player/runtime/asset.manifest.json",
            "$AssetPrefix/models/player/runtime/clip-manifest.json",
            "$AssetPrefix/models/player/runtime/gothic-traveller-lod0.runtime.glb",
            "$AssetPrefix/textures/props/runtime/asset.manifest.json",
            $LicenceEntry
        )
        if ($RequireHorde162World) {
            $requiredEntries += @("$AssetPrefix/models/world/runtime/collapsed-entry/asset.manifest.json",
                                  "$AssetPrefix/models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb")
        }
        foreach ($required in $requiredEntries) {
            if ($entries -cnotcontains $required) {
                throw "Held-item package contract missing exact entry '$required' in $resolved"
            }
        }
        if ($RequireHorde162World) {
            foreach ($relative in @('asset.manifest.json', 'collapsed-entry-lod0.runtime.glb')) {
                $name = "$AssetPrefix/models/world/runtime/collapsed-entry/$relative"
                $matches = @($archive.Entries | Where-Object { $_.FullName -ceq $name })
                if ($matches.Count -ne 1) { throw "Collapsed-entry package must contain exactly one '$name'." }
                $source = Join-Path (Split-Path -Parent $PSScriptRoot) "assets/models/world/runtime/collapsed-entry/$relative"
                if ($matches[0].Length -ne (Get-Item -LiteralPath $source).Length) { throw "Collapsed-entry package byte/hash mismatch: $name" }
                $sha = [Security.Cryptography.SHA256]::Create(); $stream = $matches[0].Open()
                try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
                finally { $stream.Dispose(); $sha.Dispose() }
                if ($actual -cne (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash) { throw "Collapsed-entry package byte/hash mismatch: $name" }
            }
        }
        # A ZIP entry alone is not an admitted runtime asset. Fresh isolated
        # checkouts can package Git LFS pointer text and still build successfully.
        foreach ($entry in $archive.Entries | Where-Object {
            $_.FullName.Replace('\', '/').StartsWith("$AssetPrefix/", [StringComparison]::Ordinal) -and
            $_.FullName -match '\.(glb|ktx2|wav)$'
        }) {
            $stream = $entry.Open()
            $prefix = [byte[]]::new(64)
            try { $read = $stream.Read($prefix, 0, $prefix.Length) }
            finally { $stream.Dispose() }
            $text = [Text.Encoding]::ASCII.GetString($prefix, 0, $read)
            if ($text.StartsWith('version https://git-lfs.github.com/spec/v1', [StringComparison]::Ordinal)) {
                throw "Runtime package contains an unhydrated Git LFS pointer: $($entry.FullName)"
            }
            if ($entry.FullName.EndsWith('.glb', [StringComparison]::Ordinal) -and
                ($read -lt 12 -or $text.Substring(0, 4) -cne 'glTF' -or
                 [BitConverter]::ToUInt32($prefix, 4) -ne 2 -or
                 [BitConverter]::ToUInt32($prefix, 8) -ne $entry.Length)) {
                throw "Runtime package contains an invalid GLB header/length: $($entry.FullName)"
            }
            if ($entry.FullName.EndsWith('.wav', [StringComparison]::Ordinal) -and
                ($read -lt 12 -or $text.Substring(0, 4) -cne 'RIFF' -or $text.Substring(8, 4) -cne 'WAVE')) {
                throw "Runtime package contains an invalid WAV header: $($entry.FullName)"
            }
            if ($entry.FullName.EndsWith('.ktx2', [StringComparison]::Ordinal) -and
                ($read -lt 12 -or [BitConverter]::ToString($prefix, 0, 12) -cne 'AB-4B-54-58-20-32-30-BB-0D-0A-1A-0A')) {
                throw "Runtime package contains an invalid KTX2 header: $($entry.FullName)"
            }
        }
        $forbidden = @($entries | Where-Object {
            $_ -match '(^|/)(source|high)(/|$)' -or
            $_ -match 'models/props/meshy/production-' -or
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

Write-Output "Held-item/player runtime package entries and exact Meshy CC BY 4.0 attribution passed."
