$script:Horde17ReleaseFiles = [ordered]@{
    'private_flask/player.glb' = '56d97faeb0595a625282c3798bc40b49d7ffedd9598e2d99fd8a186acf15f014'
    'private_revival/skeleton.glb' = '800632b9b1dbe53bd4f6508a6d167e52e940c6bdb4e902af752429e96d7fe259'
    'private_revival/lich_revival.wav' = '1e1891010f4c89e6f20ec5fba90191aa0f0999c0bd95a4df3fe36659beb9e521'
}

function Assert-Horde17ReleaseAssets {
    param([Parameter(Mandatory=$true)][string]$AssetRoot)
    $root=[IO.Path]::GetFullPath($AssetRoot)
    if(-not (Test-Path -LiteralPath $root -PathType Container)) { throw 'Pinned 1.7 release asset root is missing.' }
    $actual=@(Get-ChildItem -LiteralPath $root -Recurse -File | ForEach-Object {
        [IO.Path]::GetRelativePath($root,$_.FullName).Replace('\','/')
    } | Sort-Object)
    $expected=@($script:Horde17ReleaseFiles.Keys | Sort-Object)
    if(@(Compare-Object $expected $actual).Count -ne 0) {
        throw 'Pinned 1.7 release asset root must contain exactly the three admitted runtime derivatives, without source files.'
    }
    foreach($relative in $script:Horde17ReleaseFiles.Keys) {
        $path=Join-Path $root $relative.Replace('/',[IO.Path]::DirectorySeparatorChar)
        $hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        if($hash -ne $script:Horde17ReleaseFiles[$relative]) {
            throw "Pinned 1.7 release asset differs: $relative"
        }
    }
    return $root
}

function Copy-Horde17ReleaseAssets {
    param([Parameter(Mandatory=$true)][string]$AssetRoot,
          [Parameter(Mandatory=$true)][string]$DestinationRoot)
    $root=Assert-Horde17ReleaseAssets -AssetRoot $AssetRoot
    foreach($relative in $script:Horde17ReleaseFiles.Keys) {
        $destination=Join-Path $DestinationRoot $relative.Replace('/',[IO.Path]::DirectorySeparatorChar)
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $root $relative.Replace('/',[IO.Path]::DirectorySeparatorChar)) -Destination $destination
        if((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant() -ne $script:Horde17ReleaseFiles[$relative]) {
            throw "Pinned 1.7 release stage differs: $relative"
        }
    }
}

function Assert-Horde17ReleasePackage {
    param([Parameter(Mandatory=$true)][string]$ArchivePath,
          [Parameter(Mandatory=$true)][ValidateSet('Windows','Android')][string]$Platform)
    Add-Type -AssemblyName System.IO.Compression
    $archive=[IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($ArchivePath))
    try {
        foreach($relative in $script:Horde17ReleaseFiles.Keys) {
            $entryName=if($Platform -eq 'Android'){ "assets/$relative" }else{ $relative }
            $entries=@($archive.Entries | Where-Object FullName -eq $entryName)
            if($entries.Count -ne 1) { throw "Pinned 1.7 $Platform package is missing or duplicates $entryName" }
            $stream=$entries[0].Open()
            try {
                $hash=([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))).ToLowerInvariant()
            } finally { $stream.Dispose() }
            if($hash -ne $script:Horde17ReleaseFiles[$relative]) {
                throw "Pinned 1.7 $Platform package differs: $entryName"
            }
        }
    } finally { $archive.Dispose() }
}
