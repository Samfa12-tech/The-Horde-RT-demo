param([Parameter(Mandatory=$true)][string]$PhoneRoot,
      [Parameter(Mandatory=$true)][string]$OutputPath)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputPath){throw 'Refusing to overwrite derived evidence.'}
# Finite read-only projection of existing admitted trials; no fresh admission.
$rows = foreach($phone in @('s26','s24')) {
    foreach($cohort in @('c1','p1','p2','c2')) {
        foreach($workload in @('route','high','live')) {
            $run="mobile-pane-$phone-$cohort-$workload-20261001"
            if($phone -eq 's26' -and $cohort -eq 'c1' -and $workload -eq 'route') {
                $run='mobile-pane-s26-c1-route-valid-20261001'
            }
            $directory=Join-Path $PhoneRoot $run
            # Accept either original nested collector layout or curated flat archive.
            $benchmarkPath=Join-Path $directory "$run/benchmark.json"
            if(-not(Test-Path -LiteralPath $benchmarkPath)) {
                $benchmarkPath=Join-Path $directory 'benchmark.json'
            }
            $analysisPath=Join-Path $directory 'analysis.json'
            $raw=Get-Content -LiteralPath $benchmarkPath -Raw | ConvertFrom-Json
            $admitted=Get-Content -LiteralPath $analysisPath -Raw | ConvertFrom-Json
            if($admitted.integrity -cne 'PASS' -or $admitted.runId -cne $run) {
                throw "Existing admission for $run does not identify a PASS."
            }
            $zoneName=if($workload -eq 'route'){'opening'}else{'yellow-torch-bay'}
            $zones=@($raw.completedFrameEvidence.zones | Where-Object name -CEQ $zoneName)
            if($zones.Count -ne 1){throw "Expected one $zoneName summary for $run."}
            $zone=$zones[0]
            [pscustomobject]@{
                runId=$run; window=$zone.name; frames=$zone.counts.cpuAccepted
                cycleMedianMs=$zone.cpuStages.wholeFrameCycleCpuMs.medianMilliseconds
                cycleP95Ms=$zone.cpuStages.wholeFrameCycleCpuMs.p95Milliseconds
                gpuMedianMs=$zone.gpuRtDurationMs.medianMilliseconds
                gpuP95Ms=$zone.gpuRtDurationMs.p95Milliseconds
                strategy=$admitted.actualActiveStrategyCounts
                benchmarkSha256=(Get-FileHash -LiteralPath $benchmarkPath -Algorithm SHA256).Hash.ToLowerInvariant()
                analysisSha256=(Get-FileHash -LiteralPath $analysisPath -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }
    }
}
if(@($rows).Count -ne 24){throw 'Finite projection did not produce24 windows.'}
$rows | ConvertTo-Json -Depth 6 | Out-File -LiteralPath $OutputPath -Encoding utf8
Write-Host 'Projected24 already-admitted windows; no new test/acceptance claimed.'
