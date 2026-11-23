# Explicit small roster: the unadmitted cicada candidate is never copied.
function Get-HordeNightAudioRuntimeFiles {
    @('audio/night-ambience/runtime/tawny_owl.wav',
      'audio/night-ambience/runtime/evening_cricket.wav',
      'audio/night-ambience/runtime/pond_frogs_crickets.wav',
      'audio/night-ambience/manifest.json', 'audio/night-ambience/NOTICE.md',
      'audio/chapter-local-ambience/asset.manifest.json',
      'audio/chapter-local-ambience/runtime/tomb_drip.wav',
      'audio/chapter-local-ambience/runtime/rope_contact.wav',
      'audio/bellwether-hearth/asset.manifest.json',
      'audio/bellwether-hearth/runtime/hearth_crackle.wav',
      'audio/bellwether-stock/asset.manifest.json',
      'audio/bellwether-stock/runtime/pig_grunt.wav',
      'audio/bellwether-stock/runtime/hen_cluck.wav',
      'audio/kit-ankle/asset.manifest.json',
      'audio/kit-ankle/runtime/boot_and_gravel.wav',
      'audio/kit-ankle/runtime/steady_breath.wav',
      'audio/forest-foliage/asset.manifest.json',
      'audio/forest-foliage/runtime/leaf_rustle.wav')
}

function Copy-HordeNightAudioRuntimeAssets {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot,
          [Parameter(Mandatory=$true)][string]$AssetRoot)
    $ErrorActionPreference='Stop'
    $root=[IO.Path]::GetFullPath($RepositoryRoot)
    $destination=[IO.Path]::GetFullPath($AssetRoot)
    foreach($relative in Get-HordeNightAudioRuntimeFiles) {
        $source=Join-Path (Join-Path $root 'assets') $relative
        $target=Join-Path $destination $relative
        if(Test-Path -LiteralPath $target) {
            if((Get-FileHash -LiteralPath $target).Hash -cne (Get-FileHash -LiteralPath $source).Hash) {
                throw "Refusing to overwrite a different staged nature asset: $relative"
            }
        } else {
            $null=New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force
            Copy-Item -LiteralPath $source -Destination $target
        }
    }
}
