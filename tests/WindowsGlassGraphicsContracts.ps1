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
Write-Output "PASS: Windows glass graphics source contracts; $script:checks checks. No Vulkan/GUI/device execution."
