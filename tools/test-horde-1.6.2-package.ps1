param(
    [Parameter(Mandatory=$true)][string]$ArchivePath,
    [Parameter(Mandatory=$true)][ValidateSet('Windows','Android')][string]$Platform,
    [string]$RepositoryRoot = (Join-Path $PSScriptRoot '..')
)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'horde-1.6.2-asset-policy.ps1')
Assert-Horde162Package -RepositoryRoot $RepositoryRoot -ArchivePath $ArchivePath -Platform $Platform
Write-Output "PASS: $Platform 1.6.2 runtime audio/environment package roster and exact bytes."
Write-Output 'This is asset admission only; signing, device, visual and changed-audio acceptance are separate.'
