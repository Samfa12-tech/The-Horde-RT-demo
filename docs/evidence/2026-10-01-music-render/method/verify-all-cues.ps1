$ErrorActionPreference = 'Stop'
$root = 'C:\Dev\tmp\horde-music-render-preparation-20260930\all-cue-render-corrected'
$repo = 'C:\Users\sam_s\Documents\Pocket Chordsmith'
$htmlPath = Join-Path $repo 'apps\chordsmith-web\pocket_chordsmith_v68_core_bridge.html'
$zipPath = 'C:\Users\sam_s\Downloads\What_the_Dark_Keeps_Horde_RT_Music_Pack.zip'
$adapterPath = Join-Path $root 'render-sections.cjs'
$receipt = Get-Content -Raw -LiteralPath (Join-Path $root 'all-cues.json') | ConvertFrom-Json -AsHashtable
$expected = @{
  html = 'b266814fff749bd4d7be9d8725e4d7becc2d944122bc2a302602457fa9b6e2cf'
  ownerArchive = 'e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa'
  score = 'bc092a0f7489e52ab1e7e55e42c813ae58517ea4a73e6808de8fbbc71595d4a6'
}
function Get-Hash($Path) { (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant() }
function Assert($Condition, $Message) { if (-not $Condition) { throw $Message } }

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
  $scoreEntry = $archive.GetEntry('What_the_Dark_Keeps_Pocket_Chordsmith.json')
  Assert ($null -ne $scoreEntry) 'Score ZIP entry missing'
  $stream = $scoreEntry.Open()
  try { $memory = [IO.MemoryStream]::new(); $stream.CopyTo($memory); $scoreBytes = $memory.ToArray(); $memory.Dispose() } finally { $stream.Dispose() }
} finally { $archive.Dispose() }
$actualSource = @{
  html = Get-Hash $htmlPath
  ownerArchive = Get-Hash $zipPath
  score = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($scoreBytes)).ToLowerInvariant()
  adapter = Get-Hash $adapterPath
}
foreach ($key in $expected.Keys) { Assert ($actualSource[$key] -ceq $expected[$key]) "Current $key hash differs from pinned source" }
foreach ($key in @('html','ownerArchive','score','adapter')) {
  Assert ($actualSource[$key] -ceq $receipt.source.provenanceBefore[$key]) "Independent pre-render $key identity differs"
  Assert ($actualSource[$key] -ceq $receipt.source.provenanceAfter[$key]) "Independent post-render $key identity differs"
}
Assert ($receipt.source.provenanceStable -eq $true) 'Renderer did not certify stable provenance'
Assert ($receipt.source.priorReceiptHtmlHashStatus -like 'mismatch:*') 'Original receipt red provenance discrepancy was not retained'
Assert ($receipt.source.priorReceiptHtmlHash -cne $expected.html) 'Original receipt hash mismatch was concealed'

$score = [Text.Encoding]::UTF8.GetString($scoreBytes) | ConvertFrom-Json -AsHashtable
$framesByCue = @{ A=576000; B=576000; C=144000; D=576000; E=576000; F=576000; G=288000; H=576000 }
$expectedCueOrder = @('A','B','C','D','E','F','G','H')
Assert (($receipt.renders.Count -eq 8) -and ((@($receipt.renders | ForEach-Object cue) -join ',') -ceq ($expectedCueOrder -join ','))) 'Cue list/count/order mismatch'
$results = [Collections.Generic.List[object]]::new()
foreach ($row in $receipt.renders) {
  $cue = [string]$row.cue
  $r = $row.render
  Assert ($r.errors.Count -eq 0) "$cue has renderer errors"
  Assert ($row.pageErrors.Count -eq 0) "$cue has browser page errors"
  Assert ($r.audio.sampleRate -eq 48000 -and $r.audio.channels -eq 2) "$cue audio format mismatch"
  $frames = $framesByCue[$cue]
  Assert ($r.audio.bodyFrames -eq $frames -and $r.audio.tailFrames -eq 144000) "$cue body/tail frame length mismatch"
  Assert ($r.sourceMask.melodyStartsMatch -and $r.sourceMask.holdsMatch) "$cue source/imported masks mismatch"
  $tracks = @($score["melodyTracks$cue"])
  $holds = @($score["melodyHold$cue"])
  Assert ($r.sourceMask.rawMelodyStarts.Count -eq $tracks.Count -and $r.sourceMask.rawHoldIndices.Count -eq $holds.Count) "$cue score track count mismatch"
  $startTotal = 0; $holdTotal = 0; $repeatTotal = 0
  for ($trackIndex=0; $trackIndex -lt $tracks.Count; $trackIndex++) {
    $expectedStarts = [Collections.Generic.List[object]]::new()
    $expectedHolds = [Collections.Generic.List[int]]::new()
    $previousStart = $null
    for ($step=0; $step -lt $tracks[$trackIndex].Count; $step++) {
      $isHeld = ($trackIndex -lt $holds.Count -and $step -lt $holds[$trackIndex].Count -and [bool]$holds[$trackIndex][$step])
      if ($isHeld) { $expectedHolds.Add($step); $holdTotal++; continue }
      $degree = $tracks[$trackIndex][$step]
      if ($null -eq $degree) { continue }
      $expectedStarts.Add(@{ step=$step; degree=$degree }); $startTotal++
      if ($null -ne $previousStart -and $previousStart.degree -eq $degree) { $repeatTotal++ }
      $previousStart = @{ step=$step; degree=$degree }
    }
    $actualStarts = @($r.sourceMask.rawMelodyStarts[$trackIndex])
    $actualHolds = @($r.sourceMask.rawHoldIndices[$trackIndex])
    Assert ($actualStarts.Count -eq $expectedStarts.Count) "$cue track $trackIndex score start count mismatch"
    for ($i=0; $i -lt $expectedStarts.Count; $i++) { Assert ($actualStarts[$i].step -eq $expectedStarts[$i].step -and $actualStarts[$i].degree -eq $expectedStarts[$i].degree) "$cue track $trackIndex start mask differs from score" }
    Assert ($actualHolds.Count -eq $expectedHolds.Count) "$cue track $trackIndex score hold count mismatch"
    for ($i=0; $i -lt $expectedHolds.Count; $i++) { Assert ($actualHolds[$i] -eq $expectedHolds[$i]) "$cue track $trackIndex hold mask differs from score" }
  }
  Assert ($r.eventTrace.repeatedPitchStarts.Count -eq $repeatTotal) "$cue repeated-pitch pairs differ from score"
  Assert ($r.calls.leadCallsMatch -and $r.calls.chordCallsMatch -and $r.calls.bassCallsMatch) "$cue live scheduler trace mismatch"
  Assert ($r.calls.actualLeadPhraseCalls.Count -eq $startTotal) "$cue live lead event count differs from score"
  Assert ($r.calls.expectedChordCount -eq $r.calls.actualChordCalls.Count) "$cue chord call count mismatch"
  Assert ($r.calls.expectedBassCalls.Count -eq $r.calls.actualBassPhraseCalls.Count) "$cue bass call count mismatch"
  foreach ($key in @('kick','snare','hat','expanded')) { Assert ($r.calls.drumCountsMatch[$key] -eq $true -and $r.calls.expectedDrumCounts[$key] -eq $r.calls.actualDrumCounts[$key]) "$cue $key trigger count mismatch" }
  Assert ($r.audio.metrics.nonFiniteSamples -eq 0 -and $r.audio.metrics.clippedSamplesAtPcm16Ceiling -eq 0) "$cue has nonfinite or ceiling-clipped source samples"
  Assert ($r.audio.fxNodes.reverb -and $r.audio.fxNodes.delay -and $r.audio.fxNodes.chorus -and $r.audio.fxNodes.flanger -and $r.audio.fxNodes.limiter) "$cue missing a live FX node"
  if ($cue -eq 'G') { Assert ($r.schedule.firstFsharp4.Count -gt 0 -and $r.schedule.firstFsharp4[0].midi -eq 66 -and [Math]::Abs($r.schedule.firstFsharp4[0].timeFromMusicalStartSeconds - 4.5) -lt 1e-9) 'G first F#4 timing mismatch' }
  $wavRecords = @{}
  foreach ($name in @("$cue-loop.wav", "$cue-tail.wav")) {
    $file = Join-Path $root $name
    Assert (Test-Path -LiteralPath $file) "$cue missing $name"
    $bytes = [IO.File]::ReadAllBytes($file)
    Assert ($bytes.Length -ge 44 -and [Text.Encoding]::ASCII.GetString($bytes,0,4) -ceq 'RIFF' -and [Text.Encoding]::ASCII.GetString($bytes,8,4) -ceq 'WAVE') "$cue invalid WAV header: $name"
    Assert ([BitConverter]::ToUInt16($bytes,22) -eq 2 -and [BitConverter]::ToUInt32($bytes,24) -eq 48000 -and [BitConverter]::ToUInt16($bytes,34) -eq 16) "$cue WAV format mismatch: $name"
    $expectedFrames = if ($name.EndsWith('-loop.wav')) { $frames } else { 144000 }
    Assert ([BitConverter]::ToUInt32($bytes,40) -eq $expectedFrames * 4 -and $bytes.Length -eq 44 + $expectedFrames * 4) "$cue WAV data length mismatch: $name"
    $match = @($row.outputs | Where-Object path -CEQ $name)
    Assert ($match.Count -eq 1 -and (Get-Hash $file) -ceq $match[0].sha256) "$cue WAV receipt hash mismatch: $name"
    $wavRecords[$name] = @{ bytes=$bytes.Length; frames=$expectedFrames; sha256=(Get-Hash $file) }
  }
  $results.Add(@{ cue=$cue; frames=$frames; melodyStarts=$startTotal; holdCells=$holdTotal; repeatedPitchPairs=$repeatTotal; bodyPeak=$r.audio.metrics.bodyPeak; tailPeak=$r.audio.metrics.tailPeak; bodyRms=$r.audio.metrics.bodyRms; tailRms=$r.audio.metrics.tailRms; seamMaxAbsDifference=$r.audio.metrics.seamMaxAbsDifference; wavs=$wavRecords })
}
$report = @{ status='PASS'; scope='Local investigation-only WAV/provenance/score/scheduler verification; not listening or release admission'; source=$actualSource; cues=$results }
$report | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $root 'verification.json') -Encoding utf8
$report | ConvertTo-Json -Depth 12
