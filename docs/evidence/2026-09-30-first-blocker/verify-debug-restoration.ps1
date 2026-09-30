param([Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
$run=Join-Path $PSScriptRoot 'phone-images-restored-retry/run-20260930-204834'
$baseline='C:/Dev/tmp/horde-glass-live-20260930/derived-normal-bias-01/phone/run-20260930-152111'
function LoadJson([string]$path){ Get-Content -LiteralPath $path -Raw | ConvertFrom-Json -Depth 100 }
function Sha([string]$path){ (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Require([bool]$condition,[string]$message){ if(-not $condition){throw $message} }
$manifest=LoadJson (Join-Path $run 'capture-manifest.json')
$summary=LoadJson (Join-Path $run 'summary.json')
$state=LoadJson (Join-Path $run 'capture-01-lantern-glass-production-state.json')
$oldState=LoadJson (Join-Path $baseline 'capture-01-lantern-glass-production-state.json')
$apk='389d6954f7a3fddde1ced690470029fa4b14cc6f825fa34bf7269dc617f946e0'
Require ($manifest.device.model -eq 'SM-S948B' -and $manifest.device.serial -eq 'R5GL219SZGK') 'Wrong device'
Require ($manifest.apkSha256 -eq $apk -and $manifest.installedApkSha256 -eq $apk -and $summary.apkSha256 -eq $apk -and $summary.installedApkSha256 -eq $apk) 'Wrong restored APK'
Require ($summary.failures.Count -eq 0 -and $summary.mode -eq 'Replay' -and $summary.scale -eq 75 -and $summary.lifecycle.homeResumePassed -and $summary.lifecycle.honestPresentationAfterResume) 'Restoration runner failed'
Require ($manifest.checkpointCount -eq 1 -and $manifest.checkpoints[0].checkpoint -eq 'lantern-glass-production') 'Wrong capture'
$png='capture-01-lantern-glass-production-75.png'
$restoredHash=Sha (Join-Path $run $png)
$baselineHash=Sha (Join-Path $baseline $png)
Require ($restoredHash -eq $baselineHash -and $restoredHash -eq $manifest.checkpoints[0].png.sha256) 'Restored PNG differs'
$frame=$state.rtFrameEvidence.completedFrame
Require ($state.presented -and $state.executionBackend -eq 'RayTracingPipeline' -and $state.renderScale -eq 0.75 -and $state.internalExtent.width -eq 1080 -and $state.internalExtent.height -eq 2235) 'Wrong capture presentation/extent'
Require ($frame.dielectric.status -eq 'valid' -and $frame.gpu.status -eq 'valid' -and $frame.presentation.presented) 'Missing owning completion'
Require ($frame.identity.submissionSerial -eq $frame.identity.completionSerial -and $frame.dielectric.completedSubmissionSerial -eq $frame.identity.submissionSerial -and $frame.gpu.completedSubmissionSerial -eq $frame.identity.submissionSerial) 'Ownership mismatch'
Require ($frame.pipeline.activeStrategy -eq 'generic-dielectric' -and $frame.pipeline.genericDielectric.sha256 -eq '9865690c11de315c1c4e49af7d14eb8963b24dc522abfc01c53979bb1a5f5a91' -and $frame.pipeline.opaqueFast.sha256 -eq '8dbebd86eeed23b388626719e6f9946b8027df754869b5811673e24036ae12c9') 'Wrong restored modules'
$counterNames=@('primaryInterfaceBudgetCount','primaryInterfaceBudgetClosedVolumeCount','primaryCertifiedClosedVolumeRecoveryCount','certifiedClosedVolumeRecoveryReasonMask')
$counters=[ordered]@{}
foreach($name in $counterNames){
    Require ($state.$name -eq $oldState.$name) "Counter changed: $name"
    $counters[$name]=$state.$name
}
Require ($counters.primaryInterfaceBudgetCount -eq 1 -and $counters.primaryInterfaceBudgetClosedVolumeCount -eq 1 -and $counters.primaryCertifiedClosedVolumeRecoveryCount -eq 80 -and $counters.certifiedClosedVolumeRecoveryReasonMask -eq 2) 'Unexpected aggregate glass witness'
$receipt=[ordered]@{
    schema=1; result='pass'; deviceModel='SM-S948B'; serial='R5GL219SZGK'; run='run-20260930-204834'
    apkSha256=$apk; installedApkSha256=$manifest.installedApkSha256
    runnerSourceCommit=$manifest.sourceCommit; runnerSourceDirty=$manifest.sourceDirty
    artifactProvenance='Reused immutable derived-normal-bias-01 Debug APK, not a new clean build from the runner worktree commit.'
    backend=$state.executionBackend; scale=75; sceneOnly=$manifest.checkpoints[0].sceneOnly
    replayAndHomeResume='pass'; capturedOwningIdentity=$frame.identity
    restoredPngSha256=$restoredHash; historicalSameArtifactPngSha256=$baselineHash; exactPngEqual=$true
    diagnosticAggregateCounters=$counters
    inputs=@('capture-manifest.json','summary.json','capture-01-lantern-glass-production-state.json','route-replay-state.json','vulkan_capability_report.json') | ForEach-Object { [ordered]@{path=$_;sha256=(Sha (Join-Path $run $_))} }
    retainedEarlierAttempt='run-20260930-204031 timed out at the original 120-second operational deadline. Retry used the existing TimeoutSeconds 300 option; no count, pixel, performance or physics gates changed.'
    limits=@('Historical image equality and aggregate counters do not close current glass path, live-motion or physical-shadow gates.','Single Diagnostic frame is not a Shipping performance measurement.','No phone compute, S24/S25 or backend parity certification.','No phone operations performed by this offline verifier.')
}
Require (-not (Test-Path -LiteralPath $Output)) 'Refuse to overwrite restoration receipt'
$receipt | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $Output -Encoding utf8NoBOM
Write-Output 'Offline Debug restoration verification PASS: exact APK/presentation/PNG/counters/owning identity.'
