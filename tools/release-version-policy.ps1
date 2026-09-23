$script:HordeLatestPublishedAlphaPatch = 5
$script:HordeLatestPublishedVersionCode = 8
. (Join-Path $PSScriptRoot "version-contract.ps1")

function Assert-HordeReleaseVersionIsMutable {
    param(
        [Parameter(Mandatory = $true)][string]$Version,
        [Parameter(Mandatory = $true)][int]$VersionCode
    )

    $immutablePattern = '^(?:0\.1\.(?:1|2|3|4|5)|1\.5\.2|1\.6\.0)(?:$|[-+.])'
    if ($Version -match $immutablePattern) {
        $action = "Choose a new Version."
        throw "The published 0.1.1 through 0.1.$script:HordeLatestPublishedAlphaPatch, 1.5.2, and 1.6.0 release lines are immutable. $action"
    }
    if ($VersionCode -le $script:HordeLatestPublishedVersionCode) {
        throw "VersionCode must be greater than the immutable published value $script:HordeLatestPublishedVersionCode."
    }
    Assert-HordeSourceIdentityMatches -Version $Version -VersionCode $VersionCode | Out-Null
}
