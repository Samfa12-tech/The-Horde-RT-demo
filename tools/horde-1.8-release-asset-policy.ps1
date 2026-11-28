function Get-Horde18ResidentPins {
    param([Parameter(Mandatory)][ValidateSet('Windows','Android')][string]$Platform)

    $receipt = Get-Content -LiteralPath (Join-Path $PSScriptRoot '../docs/evidence/2026-11-28-bellwether-owner-intake/receipt.json') -Raw | ConvertFrom-Json -AsHashtable
    $pinned = [ordered]@{}
    foreach ($resident in $receipt.residents.GetEnumerator()) {
        $prefix = $resident.Key
        $pinned["$prefix/resident.glb"] = $resident.Value.glbSha256
        foreach ($file in $resident.Value.platforms[$Platform.ToLowerInvariant()].GetEnumerator()) {
            $pinned["$prefix/$($file.Key)"] = $file.Value
        }
    }
    return $pinned
}

function Get-Horde18VoicePins {
    $root = Join-Path $PSScriptRoot '../docs/evidence/2026-11-28-bellwether-owner-intake'
    $intake = Get-Content -LiteralPath (Join-Path $root 'receipt.json') -Raw | ConvertFrom-Json -AsHashtable
    $archive = Get-Content -LiteralPath (Join-Path $root 'lantern-archive.json') -Raw | ConvertFrom-Json -AsHashtable
    $pinned = [ordered]@{}
    foreach ($take in $intake.voiceTakes.GetEnumerator()) { $pinned["$($take.Key).wav"] = $take.Value.sha256 }
    foreach ($take in $archive.takes.GetEnumerator()) { $pinned.Add("$($take.Key).wav", $take.Value.runtimeSha256) }
    return $pinned
}

function Assert-Horde18AssetRoot {
    param([Parameter(Mandatory)][string]$Root,
          [Parameter(Mandatory)][System.Collections.IDictionary]$Pins,
          [string]$Prefix = '')

    $absolute = (Resolve-Path -LiteralPath $Root -ErrorAction Stop).Path
    if (!(Test-Path -LiteralPath $absolute -PathType Container)) { throw 'Bellwether asset root must be a directory.' }
    $actual = @(Get-ChildItem -LiteralPath $absolute -Recurse -File -Force | ForEach-Object {
        [IO.Path]::GetRelativePath($absolute, $_.FullName).Replace('\','/')
    })
    $expected = @($Pins.Keys | ForEach-Object { "$Prefix$_" })
    if ($actual.Count -ne $expected.Count -or @(Compare-Object $expected $actual -CaseSensitive).Count -ne 0) {
        throw 'Bellwether asset root must contain exactly the pinned runtime files.'
    }
    foreach ($relative in $Pins.Keys) {
        if ((Get-FileHash -LiteralPath (Join-Path $absolute "$Prefix$relative") -Algorithm SHA256).Hash -ne $Pins[$relative]) {
            throw "Bellwether asset differs: $Prefix$relative"
        }
    }
    return $absolute
}

function Get-Horde18ReleaseInputs {
    param([Parameter(Mandatory)][string]$WindowsResidents,
          [Parameter(Mandatory)][string]$AndroidResidents,
          [string]$Voices)

    $windows = Get-Horde18ResidentPins Windows
    $android = Get-Horde18ResidentPins Android
    $voicePins = if ($Voices) { Get-Horde18VoicePins } else { [ordered]@{} }
    return [pscustomobject]@{
        WindowsResidents = Assert-Horde18AssetRoot -Root $WindowsResidents -Pins $windows
        AndroidResidents = Assert-Horde18AssetRoot -Root $AndroidResidents -Pins $android -Prefix 'private_bellwether/'
        Voices = if ($Voices) { Assert-Horde18AssetRoot -Root $Voices -Pins $voicePins -Prefix 'private_bellwether_voice/' } else { '' }
    }
}

function Copy-Horde18WindowsAssets {
    param([Parameter(Mandatory)]$Inputs, [Parameter(Mandatory)][string]$PackageRoot)

    foreach ($relative in (Get-Horde18ResidentPins Windows).Keys) {
        $destination = Join-Path $PackageRoot "assets/private_bellwether/$relative"
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
        Copy-Item -LiteralPath (Join-Path $Inputs.WindowsResidents $relative) -Destination $destination
    }
    if ($Inputs.Voices) {
        $destination = Join-Path $PackageRoot 'assets/private_bellwether_voice'
        New-Item -ItemType Directory -Force -Path $destination | Out-Null
        foreach ($relative in (Get-Horde18VoicePins).Keys) {
            Copy-Item -LiteralPath (Join-Path $Inputs.Voices "private_bellwether_voice/$relative") -Destination $destination
        }
    }
}

function Assert-Horde18ReleasePackage {
    param([Parameter(Mandatory)][string]$ArchivePath,
          [Parameter(Mandatory)][ValidateSet('Windows','Android')][string]$Platform,
          [switch]$IncludeVoices)

    $expected = [System.Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
    $residents = Get-Horde18ResidentPins $Platform
    foreach ($relative in $residents.Keys) { $expected.Add("assets/private_bellwether/$relative", $residents[$relative]) }
    if ($IncludeVoices) {
        $voices = Get-Horde18VoicePins
        foreach ($relative in $voices.Keys) { $expected.Add("assets/private_bellwether_voice/$relative", $voices[$relative]) }
    }
    Add-Type -AssemblyName System.IO.Compression
    $archive = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $ArchivePath).Path)
    try {
        foreach ($entry in $archive.Entries) {
            $name = $entry.FullName
            if (!$entry.Name -or $name -notmatch '^assets/private_bellwether(?:_voice)?/') { continue }
            if (!$expected.ContainsKey($name)) { throw "Unexpected or duplicate Bellwether asset in $Platform package: $name" }
            $stream = $entry.Open()
            try { $hash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
            finally { $stream.Dispose() }
            if ($hash -ne $expected[$name]) { throw "Bellwether asset differs in $Platform package: $name" }
            $null = $expected.Remove($name)
        }
        if ($expected.Count -ne 0) { throw "Missing pinned Bellwether assets from $Platform package: $($expected.Keys -join ', ')" }
    } finally { $archive.Dispose() }
}
