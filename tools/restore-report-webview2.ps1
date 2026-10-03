[CmdletBinding()]
param(
    [string]$DependencyRoot = (Join-Path $PSScriptRoot '../build/dependencies'),
    [string]$ArchivePath = ''
)

$ErrorActionPreference = 'Stop'
$manifestPath = Join-Path $PSScriptRoot '../third_party/webview2-sdk/manifest.json'
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $manifestPath).Hash.ToLowerInvariant() -ne
    '0e4039382c34cf206401d44b6a8ec02fb9b43f42cf5834bd559cae0629c67b7e') {
    throw 'WebView2 SDK manifest pin mismatch.'
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.package -ne 'Microsoft.Web.WebView2' -or $manifest.version -ne '1.0.4258.31' -or
    $manifest.archiveSha256 -ne '56f7f4b8bf9aee4b8efefbbdd4f67d5f74ebd1b100ed0806da71bf76af481aa9' -or
    $manifest.archiveBytes -ne 9801922 -or $manifest.files.Count -ne 6) {
    throw 'WebView2 SDK pin does not match the reviewed package.'
}
$sdkRoot = [IO.Path]::GetFullPath((Join-Path $DependencyRoot "Microsoft.Web.WebView2/$($manifest.version)"))
$sdkPrefix = $sdkRoot.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
New-Item -ItemType Directory -Path $sdkRoot -Force | Out-Null
if ([string]::IsNullOrEmpty($ArchivePath)) {
    $ArchivePath = Join-Path $sdkRoot 'Microsoft.Web.WebView2.nupkg'
    if (-not (Test-Path -LiteralPath $ArchivePath)) {
        # Developer/CI restore only: no runtime SDK or automatic Runtime install.
        Invoke-WebRequest -Uri $manifest.url -OutFile $ArchivePath
    }
}
if ((Get-Item -LiteralPath $ArchivePath).Length -ne $manifest.archiveBytes -or
    (Get-FileHash -Algorithm SHA256 -LiteralPath $ArchivePath).Hash.ToLowerInvariant() -ne $manifest.archiveSha256) {
    throw 'WebView2 SDK archive hash/size mismatch. Existing files were not replaced.'
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($ArchivePath))
try {
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($file in $manifest.files) {
        if (-not $seen.Add($file.path) -or $file.path -match '(^/|\\|(^|/)\.\.(/|$))') {
            throw 'Invalid SDK entry path.'
        }
        $destination = [IO.Path]::GetFullPath((Join-Path $sdkRoot $file.path))
        if (-not $destination.StartsWith($sdkPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'SDK entry escaped its explicit dependency directory.'
        }
        if (-not (Test-Path -LiteralPath $destination)) {
            $entry = $archive.GetEntry($file.path)
            if ($null -eq $entry -or $entry.Length -ne $file.bytes) { throw 'Pinned SDK entry is missing or has changed size.' }
            New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($destination)) -Force | Out-Null
            $input = $entry.Open()
            try {
                $output = [IO.File]::Open($destination, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write)
                try { $input.CopyTo($output) } finally { $output.Dispose() }
            } finally { $input.Dispose() }
        }
        if ((Get-Item -LiteralPath $destination).Length -ne $file.bytes -or
            (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash.ToLowerInvariant() -ne $file.sha256) {
            throw "Pinned SDK entry hash/size mismatch: $($file.path). Existing files were not replaced."
        }
    }
} finally { $archive.Dispose() }
Write-Output "Verified WebView2 $($manifest.version) SDK (six pinned entries): $sdkRoot"
