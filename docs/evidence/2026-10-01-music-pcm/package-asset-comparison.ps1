param([Parameter(Mandatory=$true)][string]$Candidate,[Parameter(Mandatory=$true)][string]$Control,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Get-Assets([string]$Path) {
    $zip=[IO.Compression.ZipFile]::OpenRead($Path)
    $rows=[ordered]@{}
    try {
        foreach($entry in $zip.Entries | Where-Object { $_.FullName.StartsWith('assets/') -and -not $_.FullName.EndsWith('/') }) {
            $s=$entry.Open()
            try { $sha=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s)).ToLowerInvariant() }
            finally { $s.Dispose() }
            $rows[$entry.FullName]=[ordered]@{bytes=$entry.Length;sha256=$sha}
        }
    } finally { $zip.Dispose() }
    return $rows
}
$a=Get-Assets $Control; $b=Get-Assets $Candidate
if(($a.Keys -join ',') -cne ($b.Keys -join ',')) {throw 'Package assets differ'}
$differences=@()
foreach($name in $a.Keys) {if($a[$name].sha256 -cne $b[$name].sha256 -or $a[$name].bytes -ne $b[$name].bytes){$differences += $name}}
$receipt=[ordered]@{
    status=if($differences.Count){'DIFFERENT'}else{'PASS'};controlApkSha256=(Get-FileHash -LiteralPath $Control).Hash.ToLowerInvariant()
    candidateApkSha256=(Get-FileHash -LiteralPath $Candidate).Hash.ToLowerInvariant();countEach=$a.Count
    changed=$differences;assets=$b;classification='APK asset bytes only; no install, playback or device acceptance'
}
$receipt|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $Output -Encoding utf8
Write-Output "APK assets: $($a.Count) entries; $($differences.Count) changed."
if($differences.Count){throw 'Candidate changes packaged assets'}
