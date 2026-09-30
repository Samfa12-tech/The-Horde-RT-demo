[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($PSScriptRoot)
$fixtureParent=Join-Path $root 'comparison-fixtures\synthetic-v3'
if(Test-Path -LiteralPath $fixtureParent){throw "Refusing to reuse existing comparator fixture tree: $fixtureParent"}
$positive=Join-Path $fixtureParent 'synthetic-positive'
$negative=Join-Path $fixtureParent 'synthetic-negative'
$comparator=Join-Path $root 'compare-trials.ps1'
$analyzerFixture=Join-Path $root 'phone\fixtures-v2'
$runIds=@('opaque-compiler-control-a1-20261001','opaque-compiler-profile-b1-20261001','opaque-compiler-profile-b2-20261001','opaque-compiler-control-a2-20261001')
function Write-Json([string]$Path,$Value){$parent=Split-Path -Parent $Path;if(-not(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent -Force|Out-Null};[IO.File]::WriteAllText($Path,(ConvertTo-Json -InputObject $Value -Depth 20)+"`n",[Text.UTF8Encoding]::new($false))}
function Seed-Fixture([string]$RootPath){
    for($i=0;$i -lt $runIds.Count;$i++){
        $id=$runIds[$i];$candidate=$id -match '^opaque-compiler-profile-';$case=if($candidate){'green-candidate'}else{'green-control'}
        $source=Join-Path $analyzerFixture "$case\analysis.json"
        $value=Get-Content -LiteralPath $source -Raw|ConvertFrom-Json
        $value.runId=$id;$value.buildLabel=if($candidate){'opaque-retained-profile'}else{'control'}
        $target=Join-Path $RootPath "phone\$id\analysis.json"
        Write-Json $target $value
    }
}
Seed-Fixture $positive
$positiveJson=& $comparator -ValidateOnly -EvidenceRoot $positive | Out-String
$positiveValue=$positiveJson|ConvertFrom-Json
if($positiveValue.runs.Count -ne 4 -or $positiveValue.fullRouteRows -ne 7352 -or $positiveValue.openingRows -ne 640 -or $positiveValue.evidenceClass -cne 'investigation-only-compiler-treatment'){throw 'Synthetic positive comparison classification/extent failed.'}
Seed-Fixture $negative
$badPath=Join-Path $negative "phone\$($runIds[1])\analysis.json"
$bad=Get-Content -LiteralPath $badPath -Raw|ConvertFrom-Json
$bad.physicalEvidenceClass='investigation-only-whole-strategy'
Write-Json $badPath $bad
$negativeResult=$null
try{& $comparator -ValidateOnly -EvidenceRoot $negative|Out-Null;throw 'Expected compiler-treatment classification rejection.'}catch{if($_.Exception.Message -notlike '*Compiler-treatment evidence classification failed*'){throw};$negativeResult=$_.Exception.Message}
$receipt=[ordered]@{schema=1;classification='synthetic comparator fixtures only; not actual ABBA results';positive=[ordered]@{result='PASS';runCount=$positiveValue.runs.Count;routeRows=$positiveValue.fullRouteRows;openingRows=$positiveValue.openingRows;evidenceClass=$positiveValue.evidenceClass;gainClaim=$positiveValue.gainClaim;causalConclusion=$positiveValue.causalConclusion};negative=[ordered]@{result='expected rejection';diagnostic=$negativeResult}}
$receiptPath=Join-Path $fixtureParent 'synthetic-comparator-test-receipt.json'
Write-Json $receiptPath $receipt
Get-FileHash -LiteralPath $receiptPath -Algorithm SHA256 | Select-Object Path,Hash
Write-Output 'Synthetic comparator fixtures: positive 4-run classification PASS; wrong evidence class RED; no comparison.json written.'
