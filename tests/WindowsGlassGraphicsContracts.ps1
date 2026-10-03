$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = Get-Content -LiteralPath (Join-Path $root 'src/platform/windows/DiagnosticWindow.cpp') -Raw
$script:checks = 0
function Check([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
    $script:checks++
}
function Section([string]$text, [string]$begin, [string]$end) {
    $start = $text.IndexOf($begin, [StringComparison]::Ordinal)
    $stop = $text.IndexOf($end, $start + $begin.Length, [StringComparison]::Ordinal)
    if ($start -lt 0 -or $stop -le $start) { throw "Missing function-scoped contract: $begin" }
    return $text.Substring($start, $stop - $start)
}
function Test-SceneRetirement([string]$body) {
    $idle = $body.IndexOf('vkDeviceWaitIdle(context.device)')
    $completion = $body.IndexOf('CompleteRtEvidenceAfterDeviceIdle(context, idleResult)')
    $commandReset = $body.IndexOf('vkResetCommandBuffer(commandBuffer, 0u)')
    $timerReset = $body.IndexOf('context.gpuFrameTimer.ResetAfterDeviceIdle()')
    $epoch = $body.IndexOf('RtResourceResetReason::DiagnosticResourceReplacement')
    $destroy = $body.IndexOf('context.rtScene.Destroy()')
    return $idle -ge 0 -and $idle -lt $completion -and $completion -lt $commandReset -and
        $commandReset -lt $timerReset -and $timerReset -lt $epoch -and $epoch -lt $destroy -and
        $body.Contains('if (!retireSceneReferences()) return false;')
}
$scene = Section $source 'bool ApplyPendingSceneReplacement(' 'bool ApplyPendingOutputResize('
$resize = Section $source 'bool ApplyPendingOutputResize(' 'bool RenderFrame('
$queue = Section $source 'bool QueueGraphicsCommand(' 'void FinishGraphicsFrame('
$finish = Section $source 'void FinishGraphicsFrame(' 'void OpenRtLab('
$current = Section $source 'horde::graphics::GraphicsSettings CurrentGraphicsSettings(' 'horde::graphics::GraphicsAppliedSnapshot GraphicsSnapshot('
$load = Section $source 'void LoadSettings(' '#if defined(_DEBUG)'
Check (Test-SceneRetirement $scene) 'Scene destruction requires successful idle/evidence, command/timer resets and replacement epoch.'
Check (-not (Test-SceneRetirement ($scene.Replace('vkDeviceWaitIdle(context.device)', 'fabricatedIdleSuccess')))) 'No idle substitute can prove scene retirement.'
Check (-not (Test-SceneRetirement ($scene.Replace('vkResetCommandBuffer(commandBuffer, 0u)', 'keepOldRecordedReferences')))) 'Recorded references must be invalidated before destruction.'
Check (-not (Test-SceneRetirement ($scene.Replace('if (!retireSceneReferences()) return false;', 'ignoreFailedRetirement();')))) 'Failed drain must abort before destruction/reinitialisation.'
Check ($scene.Contains('context.graphicsCommand ? context.graphicsBeforeApply : CurrentGraphicsSettings(context)') -and
    $scene.Contains('previousSettings.glassEnabled = context.rtScene.GlassEnabled()') -and
    $scene.Contains('const auto previousProfile = context.rtScene.Profile()') -and
    $scene.Contains('const auto previousBackend = context.rtScene.ExecutionBackend()')) 'Rollback captures actual prior tuple/profile/backend before destruction.'
$failed = Section $scene 'const std::string requestedFailure' 'else context.lastRtFrameError.clear();'
Check ($failed.IndexOf('if (!retireSceneReferences()) return false;') -lt $failed.IndexOf('context.rtScene.Destroy()')) 'Partial initialisation requires a fresh drain before cleanup.'
foreach ($restore in @('context.sceneProfile = previousProfile', 'context.executionBackend = previousBackend',
    'context.renderScale = previousSettings.renderScalePercent', 'previousSettings.waterQuality',
    'previousSettings.fireDetail', 'previousSettings.previewFrameCap', 'previousSettings.glassEnabled')) {
    Check ($failed.IndexOf($restore) -ge 0 -and $failed.IndexOf($restore) -lt $failed.IndexOf('if (!InitialiseRtSceneForSwapchain(context, false)) return false;')) "Rollback restores $restore before reinitialisation."
}
Check ($failed.Contains('failure.reasons = horde::graphics::GraphicsReason::ResourceFailure') -and
    $failed.Contains('failure.rtPresented = false') -and $failed.Contains('Acknowledge(failure, false)') -and
    -not $failed.Contains('SaveGraphicsRecord') -and -not $failed.Contains('SaveSettings')) 'Failure acknowledgement retains pending storage and cannot claim restored presentation.'
Check ($scene.Contains('context.renderScaleDirty = false') -and $scene.Contains('context.sceneProfileDirty = false') -and
    $scene.Contains('context.glassGeometryDirty = false') -and $scene.Contains('capabilities.rtScene.presented = false')) 'Combined replacement clears geometry/output work but waits for its own RT frame.'
Check ($resize.Contains('ResizeOutputAfterDeviceIdle') -and -not $resize.Contains('rtScene.Destroy') -and
    -not $resize.Contains('InitialiseRtSceneForSwapchain')) 'Scale-only requests retain the existing output-only allocation transaction.'
Check ($current.Contains('context.rtScene.IsReady() && context.rtScene.GlassEnabled()') -and
    -not $current.Contains('context.requestedGlassEnabled')) 'Applied glass comes only from actual ready renderer geometry.'
Check ($queue.IndexOf('command->serial <= context.graphicsSerialFloor') -ge 0 -and
    $queue.IndexOf('command->serial <= context.graphicsSerialFloor') -lt $queue.IndexOf('SaveGraphicsRecord') -and
    $queue.Contains('command->lifecycleGeneration != 1u')) 'Stale serial/generation is rejected before storage or live mutation.'
Check ($finish.Contains('!context.renderScaleDirty && !context.sceneProfileDirty && !context.glassGeometryDirty') -and
    $finish.Contains('context.rtScene.IsReady()') -and $finish.Contains('RtPresentationOutcome::Presented')) 'Success acknowledgement requires current presented output and every geometry dirty flag cleared.'
Check ($load.Contains('if (record.schema == 1u) return 1') -and $load.Contains('readGlass("confirmedGlass")') -and
    $load.Contains('readGlass("pendingGlass")') -and $load.Contains('record.schema = 0u') -and
    $load.Contains('length == 1u') -and $load.Contains("value[0] == '0' || value[0] == '1'")) 'Schema1 retains old four fields with Glass On; schema2 rejects non-boolean or missing glass values without integer coercion.'
Check ($source.Contains('createButton(kGraphicsGlassButtonId, "GLASS: ON")') -and
    $source.Contains('draft.glassEnabled = !draft.glassEnabled') -and
    $source.Contains('MoveWindow(GetDlgItem(window, kGraphicsGlassButtonId)')) 'Native glass action has visible placement and edits only the draft.'
$owner = $source.IndexOf('const bool graphicsTransition = context.sceneProfileDirty || context.glassGeometryDirty')
Check ($owner -ge 0 -and $source.IndexOf('ApplyPendingSceneReplacement(context, capabilities, timingSamples)', $owner) -lt
    $source.IndexOf('ApplyPendingOutputResize(context, capabilities, timingSamples)', $owner)) 'Owner applies combined scene work before considering output-only resize.'
$capture = Get-Content -LiteralPath (Join-Path $root 'src/platform/windows/WindowsGraphicsPreviewCapture.inl') -Raw
$settle = Section $capture 'const auto settle = ' 'const auto capturePose = '
$captureCommand = Section $capture 'const auto command = ' 'auto candidate = '
$readback = Section $capture 'const auto capturePose = ' 'context.graphicsEdit.emplace('
$completed = Section $capture 'bool CurrentCompletedGraphicsPreviewPresent(' 'bool WriteGraphicsPreviewCaptureManifest('
function Test-CaptureCompletedPresent([string]$body) {
    return $body.Contains('publication.hasCompletedEvidence') -and
        $body.Contains('evidence.identity.completionSerial != 0u') -and
        $body.Contains('evidence.identity.submitted.frame.sceneEpoch == publication.sceneEpoch') -and
        $body.Contains('evidence.identity.submitted.frame.measurementGeneration == publication.measurementGeneration') -and
        $body.Contains('evidence.presentation.outcome == horde::telemetry::RtPresentationOutcome::Presented') -and
        $body.Contains('evidence.presentation.lastSuccessfulPresentSubmissionSerial >= evidence.identity.submitted.submissionSerial')
}
function Test-CaptureExactAck([string]$body) {
    return $body.Contains('snapshot.serial != request->serial') -and
        $body.Contains('snapshot.lifecycleGeneration != request->lifecycleGeneration') -and
        $body.Contains('snapshot.requested == request->requested') -and
        $body.Contains('snapshot.effective == request->requested') -and
        $body.Contains('CurrentCompletedGraphicsPreviewPresent(after)') -and
        $body.Contains('after.sceneEpoch <= before.sceneEpoch') -and
        $body.Contains('request->requested.glassEnabled ? masks[9u] == 0u : masks[9u] != 0u')
}
Check ($settle.IndexOf('ApplyPendingSceneReplacement(context, capabilities, replacementTimingSamples)') -ge 0 -and
    $settle.IndexOf('ApplyPendingSceneReplacement') -lt $settle.IndexOf('RenderFrame(context') -and
    $settle.IndexOf('RenderFrame(context') -lt $settle.IndexOf('FinishGraphicsFrame(context, presented)')) 'Capture uses the ordinary owner replacement, rendering and current-frame acknowledgement boundary.'
Check (Test-CaptureCompletedPresent $completed) 'Completed capture evidence joins actual ordinary presentation with the owning current resource and measurement epochs.'
Check (-not (Test-CaptureCompletedPresent ($completed.Replace('evidence.identity.submitted.frame.sceneEpoch == publication.sceneEpoch', 'acceptOldResourceEpoch')))) 'Negative capture source fixture rejects old-resource completed evidence.'
Check (-not (Test-CaptureCompletedPresent ($completed.Replace('evidence.presentation.outcome == horde::telemetry::RtPresentationOutcome::Presented', 'acceptSuboptimalBeforeRecreate')))) 'Negative capture source fixture rejects recreated/suboptimal output as ordinary current presentation.'
Check (Test-CaptureExactAck $captureCommand) 'Transaction requires exact fresh request/effective serial/generation, completed current presentation, changed resource epoch and all-ray glass mask.'
Check (-not (Test-CaptureExactAck ($captureCommand.Replace('snapshot.effective == request->requested', 'trustRequestedWithoutActualResources')))) 'Negative capture source fixture rejects requested-only acknowledgement.'
Check (-not (Test-CaptureExactAck ($captureCommand.Replace('request->requested.glassEnabled ? masks[9u] == 0u : masks[9u] != 0u', 'ignoreOffFixtureMask')))) 'Negative capture source fixture rejects Glass Off retaining a nonzero all-ray fixture mask.'
Check ($captureCommand.Contains('idle != VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context, idle)') -and
    $captureCommand.IndexOf('CompleteRtEvidenceAfterDeviceIdle') -lt $captureCommand.IndexOf('context.graphicsEdit->Confirm()') -and
    $captureCommand.Contains('context.rtScene.ExecutionBackend() != baselineBackend') -and
    $captureCommand.Contains('context.rtScene.Profile() != baselineProfile')) 'Keep needs real completed ownership and retains the backend and compact profile.'
foreach ($phase in @('apply-glass-off', 'revert-glass-on', 'keep-glass-off-memory-only', 'restore-glass-on-memory-only')) {
    Check ($capture.Contains('command("' + $phase + '"')) "Capture exercises ordinary production transaction $phase."
}
Check ($capture.Contains('capturePose("glass-on", glassPose, baseline)') -and
    $capture.Contains('capturePose("glass-off", glassPose, glassOff)') -and
    $capture.Contains('GraphicsPreviewCamera::Glass, 120u, false, false') -and
    $capture.Contains('glassOnPose.camera.pitch != glassOffPose.camera.pitch') -and
    $capture.Contains('glassOnPose.fireEmitters[1].phase != glassOffPose.fireEmitters[1].phase') -and
    $capture.Contains('kGraphicsPreviewCapturePoses')) 'Two additional A/B images use the same shared paused Glass pose while preserving all nine accepted authored captures.'
Check ($readback.Contains('CurrentCompletedGraphicsPreviewPresent(record.publication)') -and
    $readback.Contains('simulationTick != pose.tick') -and
    $readback.Contains('record.sceneGlassEnabled = context.rtScene.GlassEnabled()') -and
    $readback.Contains('record.resources = context.rtScene.ResourceInventory()')) 'Readback records actual ready glass, current completed pose and honest live allocations.'
foreach ($field in @('glassEnabled', 'sceneEpochBefore', 'sceneEpochAfter', 'sceneGlassEnabled', 'instanceMasksByCustomIndex', 'currentCompletedPresent')) {
    Check ($capture.Contains('\"' + $field + '\"')) "Manifest exposes actual glass transaction field $field."
}
Check ($capture.Contains('Glass Off retains fixed BLAS/TLAS roles and implies no allocation savings') -and
    $capture.Contains('full Showcase roof and lantern geometry remain a separate acceptance gate') -and
    -not $capture.Contains('SaveSettings(') -and -not $capture.Contains('SettingsPath(')) 'Capture makes no savings/full-scene claim and does not read/write user preference storage.'
Write-Output "PASS: Windows glass graphics source contracts; $script:checks checks. No Vulkan/GUI/device execution."
