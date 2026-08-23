$script:HordeLatestPublishedAlphaPatch = 5
$script:HordeLatestPublishedVersionCode = 6

function Assert-HordeReleaseVersionIsMutable {
    param(
        [Parameter(Mandatory = $true)][string]$Version,
        [int]$VersionCode = 0,
        [switch]$UploadOnly
    )

    $immutablePattern = '^0\.1\.(?:1|2|3|4|5)(?:$|[-+.])'
    if ($Version -match $immutablePattern) {
        $action = if ($UploadOnly) { "Refusing another upload." } else { "Choose a new Version." }
        throw "The published 0.1.1 through 0.1.$script:HordeLatestPublishedAlphaPatch release lines are immutable. $action"
    }
    if (-not $UploadOnly -and $VersionCode -le $script:HordeLatestPublishedVersionCode) {
        throw "VersionCode must be greater than the immutable published value $script:HordeLatestPublishedVersionCode."
    }
}
