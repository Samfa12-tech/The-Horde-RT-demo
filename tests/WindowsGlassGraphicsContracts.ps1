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
$persistence = Get-Content -LiteralPath (Join-Path $root 'src/platform/windows/WindowsGraphicsPersistence.h') -Raw
$loadRecord = Section $persistence 'inline std::optional<horde::graphics::GraphicsPersistenceRecord> LoadGraphicsPersistenceRecord(' '// The confirmed tuple'
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
    'previousSettings.fireDetail', 'previousSettings.previewFrameCap', 'previousSettings.glassEnabled', 'previousSettings.shadowQuality')) {
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
Check ($current.Contains('UploadedGraphicsFireDetail(context)') -and $current.Contains('UploadedGraphicsShadowQuality(context)') -and
    -not $current.Contains('context.fireDetail') -and -not $current.Contains('context.shadowQuality')) 'Applied fire and shadow derive from successful renderer uploads, not requested context fields.'
Check ($queue.IndexOf('command->serial <= context.graphicsSerialFloor') -ge 0 -and
    $queue.IndexOf('command->serial <= context.graphicsSerialFloor') -lt $queue.IndexOf('SaveGraphicsRecord') -and
    $queue.Contains('command->lifecycleGeneration != 1u')) 'Stale serial/generation is rejected before storage or live mutation.'
function Test-CurrentQualityAck([string]$body) {
    return $body.Contains('!context.renderScaleDirty && !context.sceneProfileDirty && !context.glassGeometryDirty') -and
        $body.Contains('context.rtScene.IsReady()') -and $body.Contains('context.rtScene.HasUploadedQualityControls()') -and
        $body.Contains('RtPresentationOutcome::Presented')
}
Check (Test-CurrentQualityAck $finish) 'Success acknowledgement requires current presented output, successful policy upload and every geometry dirty flag cleared.'
Check (-not (Test-CurrentQualityAck ($finish.Replace('context.rtScene.HasUploadedQualityControls()', 'trustRequestedWithoutUploadedPolicy')))) 'Negative source fixture rejects requested fire/shadow without successful owner upload.'
Check ($load.Contains('LoadGraphicsPersistenceRecord(path, context.savedGraphics)') -and
    $loadRecord.Contains('schema == 1 ? 1 : read(pending ? "pendingGlass" : "confirmedGlass", 1, true)') -and
    $loadRecord.Contains('historical ? 1 : read(pending ? "pendingShadow" : "confirmedShadow", 1, true)') -and
    $loadRecord.Contains('fire > (historical ? 1 : 2)') -and $loadRecord.Contains('shadow < 0 || shadow > 2') -and
    $loadRecord.Contains('std::from_chars') -and $loadRecord.Contains('std::to_string(value) != std::string(text.data(), length)') -and
    $loadRecord.Contains('record.schema = 0u')) 'Ordinary strict INI loader preserves schema1 glass/schema1-2 shadow migration, rejects old Low/unknown enums before narrowing and requires canonical exact numeric/boolean keys.'
Check ($persistence.Contains('write("confirmedShadow"') -and $persistence.Contains('write("pendingShadow"') -and
    $queue.Contains('context.shadowQuality = command->requested.shadowQuality') -and
    $resize.Contains('context.shadowQuality = context.graphicsBeforeApply.shadowQuality')) 'Complete shadow tuple is persisted, applied and restored after resize failure.'
Check ($source.Contains('createButton(kGraphicsShadowButtonId, "SHADOW: CURRENT")') -and
    $source.Contains('MoveWindow(GetDlgItem(window, kGraphicsShadowButtonId)') -and
    $source.Contains('draft.shadowQuality = static_cast<horde::graphics::ShadowQuality>')) 'Native shadow button has visible placement and edits only the draft.'
$render = Section $source 'bool RenderFrame(' 'void ApplyCaptureCheckpoint('
Check ($render.Contains('ResolveFireEmitterQuality(ctx.fireDetail)') -and $render.Contains('frameInputs.shadowQuality = ctx.shadowQuality') -and
    -not $render.Contains('ctx.fireDetail == horde::graphics::FireDetail::High ?')) 'Showcase and Preview share exhaustive Low/Mobile/High mapping and independent shadow frame policy.'
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
Check ($source.Contains('completedFrame.scene.actualUploadedDustQuality !=') -and
    $source.Contains('std::optional<horde::graphics::DustQuality>{context.requestedDustQuality}') -and
    $source.Contains('context.rtScene.UploadedDustQuality() !=') -and
    $source.Contains('\"cpuWork\": {\"admittedZones\":') -and
    $source.Contains('capture.dustWork.tileReferences') -and
    $source.Contains('capture.dustWork.overflowReferences')) 'Showcase stills require exact completed/uploaded Dust quality and report per-still CPU work.'
function Test-MistCompletedAck([string]$body) {
    return $body.Contains('CurrentCompletedMistEnabled(after) != std::optional<bool>{request->requested.mistEnabled}') -and
        $body.Contains('context.rtScene.UploadedMistEnabled() != std::optional<bool>{request->requested.mistEnabled}')
}
Check (Test-MistCompletedAck $captureCommand) 'Mist transactions require both actual current completed presentation and matching uploaded optional flag; false is a valid Off value.'
Check (-not (Test-MistCompletedAck ($captureCommand.Replace('CurrentCompletedMistEnabled(after)', 'trustDesiredMist')))) 'Negative Mist fixture rejects desired-only or prior incomplete publication as acknowledgement.'
Check (-not (Test-MistCompletedAck ($captureCommand.Replace('context.rtScene.UploadedMistEnabled()', 'context.rtScene.MistEnabled()')))) 'Negative Mist fixture rejects desired state replacing successful actual upload.'
Check ($readback.Contains('CurrentCompletedMistEnabled(record.publication) != std::optional<bool>{expectedSettings.mistEnabled}') -and
    $readback.Contains('context.rtScene.UploadedMistEnabled() != std::optional<bool>{expectedSettings.mistEnabled}')) 'Mist readback requires actual completed and uploaded choice, with absence never defaulted into On proof.'
function Test-DustCompletedAck([string]$body) {
    return $body.Contains('CurrentCompletedDustQuality(after) != std::optional<horde::graphics::DustQuality>{request->requested.dustQuality}') -and
        $body.Contains('context.rtScene.UploadedDustQuality() != std::optional<horde::graphics::DustQuality>{request->requested.dustQuality}')
}
Check (Test-DustCompletedAck $captureCommand) 'Dust transactions require current completed uploaded quality and matching actual renderer upload.'
Check (-not (Test-DustCompletedAck ($captureCommand.Replace('CurrentCompletedDustQuality(after)', 'trustDesiredDust')))) 'Negative Dust fixture rejects desired-only completion acknowledgement.'
Check (-not (Test-DustCompletedAck ($captureCommand.Replace('context.rtScene.UploadedDustQuality()', 'context.rtScene.DustQuality()')))) 'Negative Dust fixture rejects desired quality replacing successful actual upload.'
Check ($readback.Contains('CurrentCompletedDustQuality(record.publication) != std::optional<horde::graphics::DustQuality>{expectedSettings.dustQuality}') -and
    $readback.Contains('context.rtScene.UploadedDustQuality() != std::optional<horde::graphics::DustQuality>{expectedSettings.dustQuality}')) 'Dust still readback requires exact actual completed and uploaded quality.'
Check ($capture.Contains('static_cast<unsigned>(settings.dustQuality)') -and
    $capture.Contains('static_cast<unsigned>(*completed.scene.actualUploadedDustQuality)') -and
    $capture.Contains('\"cpuDustWork\"') -and
    $capture.Contains('work.tileReferences') -and $capture.Contains('work.overflowReferences')) 'Capture manifests expose requested/effective/completed Dust quality and per-still CPU work counters.'
foreach ($phase in @('apply-mist-off', 'revert-mist-on')) {
    Check ($capture.Contains('command("' + $phase + '"')) "Capture exercises ordinary live Mist transaction $phase."
}
Check ($capture.Contains('command("revert-mist-on", context.graphicsEdit->RequestRevert(1u), false)') -and
    $capture.Contains('capturePose("mist-on", glassPose, baseline)') -and
    $capture.Contains('capturePose("mist-off", glassPose, mistOff)') -and
    $capture.Contains('capturePose("mist-restored-on", glassPose, baseline)') -and
    $capture.Contains('mistOnCapture.pngSha256 != compared->pngSha256') -and
    $capture.Contains('on.tick != other.tick || on.timeSeconds != other.timeSeconds') -and
    $capture.Contains('Real Keeper captures remain') -and
    $capture.Contains('const auto& pose : horde::platform::windows::kGraphicsPreviewCapturePoses')) 'Three additional fixed-pose Mist images prove unchanged compact pixels and live Revert without Keep; all nine accepted poses and real Keeper appearance gate remain.'

$layout = Section $source 'void LayoutOverlayControls(HWND window, const int width, const int height)' 'void ShowControlsHelp('
$ownerDraw = Section $source 'bool IsGraphicsMenuButton(' 'void ReplaceFontProperty('
$drawItem = Section $source 'if (item && item->CtlType == ODT_BUTTON)' 'if (!sceneContext || !item || item->CtlID != kGraphicsPreviewGraphId)'
Check ($layout.Contains('ScaleForDpi(window, 506)') -and
    $layout.Contains('MoveWindow(GetDlgItem(window, kGraphicsPresetButtonId), graphicsX, y, graphicsWidth, compactHeight, TRUE)') -and
    $layout.Contains('const int dustWidth = ScaleForDpi(window, 144)')) 'Graphics preset has its own responsive full-width row and the dust toggle reserves enough horizontal room at the 420-DIP panel minimum.'
Check ($ownerDraw.Contains('GetTextExtentPoint32A') -and $ownerDraw.Contains('fittedExtent.cx <= availableWidth') -and
    $ownerDraw.Contains('ScaleForDpi(window, 9)') -and $drawItem.Contains('CreateGraphicsButtonFitFont') -and
    $drawItem.Contains('graphicsMenuButton ? 15 : 6') -and $drawItem.Contains('DrawTextA(item->hDC, text, -1')) 'Graphics menu captions are measured in the current control font, retain the complete string, and are redrawn with a DPI-scaled fit font inside a rivet-safe interior.'
foreach ($controlId in @('kGraphicsPresetButtonId', 'kGraphicsFireButtonId', 'kGraphicsShadowButtonId',
    'kGraphicsGlassButtonId', 'kGraphicsMistButtonId', 'kGraphicsDustButtonId', 'kGraphicsApplyButtonId',
    'kGraphicsConfirmButtonId', 'kGraphicsRevertButtonId', 'kGraphicsResetButtonId',
    'kGraphicsPreviewPauseId', 'kGraphicsPreviewCameraId', 'kGraphicsPreviewMotionId',
    'kGraphicsPreviewResetId', 'kWaterQualityButtonId', 'kSettingsBackButtonId')) {
    Check ($ownerDraw.Contains("case ${controlId}:")) "$controlId participates in graphics-menu full-caption fitting."
}
$fontSetup = Section $source 'void ApplyDpiScaledFonts(HWND window)' 'void ReleaseDpiScaledFonts(HWND window)'
Check ($fontSetup.Contains('DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"') -and
    $fontSetup.Contains('DEFAULT_PITCH | FF_ROMAN, "Georgia"')) 'GDI fixtures cover the runtime Segoe UI and authored Georgia plaque faces declared by the DPI font setup.'
Check ($fontSetup.Contains('CreateFontA(ScaleForDpi(window, 14)')) 'Graphics description and telemetry retain the measured 14-DIP Segoe UI font.'
foreach ($previewId in @('kGraphicsPreviewPauseId', 'kGraphicsPreviewCameraId',
    'kGraphicsPreviewMotionId', 'kGraphicsPreviewResetId')) {
    Check ($fontSetup.Contains($previewId)) "$previewId uses the DPI-scaled native UI font measured by the fit fixture."
}
Check ($layout.Contains('const int previewX = graphicsX + graphicsWidth + gap * 2') -and
    $layout.Contains('const int previewWidth = std::max(ScaleForDpi(window, 260), width - previewX - inset)') -and
    $layout.Contains('const int previewHalf = (previewWidth - gap) / 2') -and
    $layout.Contains('const int bottom = height - inset - compactHeight * 3 - gap * 2') -and
    $layout.Contains('kGraphicsPreviewMotionId), previewX, bottom + compactHeight + gap, previewWidth') -and
    $layout.Contains('kGraphicsPreviewResetId), previewX, bottom + (compactHeight + gap) * 2, previewWidth')) 'Preview-button fixtures follow the actual right-panel width, two-column first row and full-width readable Motion/Reset rows.'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class GraphicsMenuGdiMetrics {
    [StructLayout(LayoutKind.Sequential)] public struct Size { public int cx; public int cy; }
    [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr hwnd, IntPtr dc);
    [DllImport("gdi32.dll", EntryPoint="CreateFontA", CharSet=CharSet.Ansi)]
    public static extern IntPtr CreateFont(int h, int w, int e, int o, int weight, uint italic,
        uint underline, uint strike, uint charset, uint output, uint clip, uint quality,
        uint pitch, string face);
    [DllImport("gdi32.dll")] public static extern IntPtr SelectObject(IntPtr dc, IntPtr obj);
    [DllImport("gdi32.dll", CharSet=CharSet.Ansi)]
    public static extern bool GetTextExtentPoint32A(IntPtr dc, string text, int count, out Size size);
    [DllImport("gdi32.dll", CharSet=CharSet.Ansi)]
    public static extern int GetTextFaceA(IntPtr dc, int count, System.Text.StringBuilder face);
    [DllImport("user32.dll", EntryPoint="DrawTextA", CharSet=CharSet.Ansi)]
    public static extern int DrawText(IntPtr dc, string text, int count, ref Rect rect, uint format);
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int left; public int top; public int right; public int bottom; }
    [DllImport("gdi32.dll")] public static extern bool DeleteObject(IntPtr obj);
}
'@
function Test-NativeCaptionFits([string]$text, [int]$availableWidth, [int]$dpi,
    [string]$fontFace, [int]$logicalBaseFontHeight) {
    $dc = [GraphicsMenuGdiMetrics]::GetDC([IntPtr]::Zero)
    if ($dc -eq [IntPtr]::Zero) { throw 'Unable to acquire a GDI measurement context.' }
    $baseFontHeight = [Math]::Max(1, [int][Math]::Floor($logicalBaseFontHeight * $dpi / 96.0 + 0.5))
    $fontWeight = 600
    $fontPitch = 0x20
    if ($fontFace -eq 'Georgia') { $fontWeight = 700; $fontPitch = 0x10 }
    $baseFont = [GraphicsMenuGdiMetrics]::CreateFont(-$baseFontHeight,
        0, 0, 0, $fontWeight, 0, 0, 0, 1, 0, 0, 5, $fontPitch, $fontFace)
    if ($baseFont -eq [IntPtr]::Zero) { [void][GraphicsMenuGdiMetrics]::ReleaseDC([IntPtr]::Zero, $dc); throw 'Unable to create the native UI font fixture.' }
    $oldFont = [GraphicsMenuGdiMetrics]::SelectObject($dc, $baseFont)
    try {
        $resolvedFace = New-Object System.Text.StringBuilder 128
        if ([GraphicsMenuGdiMetrics]::GetTextFaceA($dc, $resolvedFace.Capacity, $resolvedFace) -gt 0) {
            $script:graphicsGdiResolvedFaces[$fontFace] = $resolvedFace.ToString()
        }
        $minimumHeight = [Math]::Max(1, [int][Math]::Floor(9 * $dpi / 96.0 + 0.5))
        for ($fontHeight = $baseFontHeight; $fontHeight -ge $minimumHeight; --$fontHeight) {
            $font = [GraphicsMenuGdiMetrics]::CreateFont(-[int]$fontHeight, 0, 0, 0,
                $fontWeight, 0, 0, 0, 1, 0, 0, 5, $fontPitch, $fontFace)
            if ($font -eq [IntPtr]::Zero) { continue }
            $selected = [GraphicsMenuGdiMetrics]::SelectObject($dc, $font)
            $extent = New-Object GraphicsMenuGdiMetrics+Size
            $measured = [GraphicsMenuGdiMetrics]::GetTextExtentPoint32A($dc, $text, $text.Length, [ref]$extent)
            [void][GraphicsMenuGdiMetrics]::SelectObject($dc, $selected)
            [void][GraphicsMenuGdiMetrics]::DeleteObject($font)
            if ($measured -and $extent.cx -le $availableWidth) { return $true }
        }
        return $false
    }
    finally {
        [void][GraphicsMenuGdiMetrics]::SelectObject($dc, $oldFont)
        [void][GraphicsMenuGdiMetrics]::DeleteObject($baseFont)
        [void][GraphicsMenuGdiMetrics]::ReleaseDC([IntPtr]::Zero, $dc)
    }
}

function Test-NativeStaticTextFits([string]$text, [int]$availableWidth, [int]$availableHeight,
    [int]$dpi, [int]$logicalFontHeight) {
    $dc = [GraphicsMenuGdiMetrics]::GetDC([IntPtr]::Zero)
    if ($dc -eq [IntPtr]::Zero) { throw 'Unable to acquire a GDI measurement context.' }
    $pixelFontHeight = [int][Math]::Floor($logicalFontHeight * $dpi / 96.0 + 0.5)
    # Positive height matches CreateFontA in ApplyDpiScaledFonts for these statics.
    $font = [GraphicsMenuGdiMetrics]::CreateFont($pixelFontHeight,
        0, 0, 0, 400, 0, 0, 0, 1, 0, 0, 5, 0x20, 'Segoe UI')
    if ($font -eq [IntPtr]::Zero) { [void][GraphicsMenuGdiMetrics]::ReleaseDC([IntPtr]::Zero, $dc); throw 'Unable to create the native static font fixture.' }
    $oldFont = [GraphicsMenuGdiMetrics]::SelectObject($dc, $font)
    try {
        $rect = New-Object GraphicsMenuGdiMetrics+Rect
        $rect.right = $availableWidth
        $measuredHeight = [GraphicsMenuGdiMetrics]::DrawText($dc, $text, -1, [ref]$rect, 0x400 -bor 0x10)
        return $measuredHeight -le $availableHeight
    }
    finally {
        [void][GraphicsMenuGdiMetrics]::SelectObject($dc, $oldFont)
        [void][GraphicsMenuGdiMetrics]::DeleteObject($font)
        [void][GraphicsMenuGdiMetrics]::ReleaseDC([IntPtr]::Zero, $dc)
    }
}

$captionFixtures = @(
    @{ row = 'preset'; text = 'PRESET: ACCEPTED BASELINE' },
    @{ row = 'fire'; text = 'FIRE: MOBILE' },
    @{ row = 'shadow'; text = 'SHADOW: HIGHER' },
    @{ row = 'glass'; text = 'GLASS: OFF' },
    @{ row = 'mist'; text = 'MIST: OFF' },
    @{ row = 'dust'; text = 'INDOOR DUST: HIGH' },
    @{ row = 'water'; text = 'RT WATER: HIGH' },
    @{ row = 'apply'; text = 'APPLY' },
    @{ row = 'confirm'; text = 'KEEP (15 SECONDS)' },
    @{ row = 'revert'; text = 'REVERT' },
    @{ row = 'reset'; text = 'DEFAULTS (DRAFT)' },
    @{ row = 'back'; text = 'BACK' },
    @{ row = 'previewPause'; text = 'PAUSE PREVIEW' },
    @{ row = 'previewCamera'; text = 'VIEW: OVERVIEW' },
    @{ row = 'previewMotion'; text = 'MOTION TEST: OFF' },
    @{ row = 'previewReset'; text = 'RESET PREVIEW TIMELINE' }
)
$script:graphicsGdiResolvedFaces = @{}
$fontFixtures = @(
    @{ face = 'Segoe UI'; logicalHeight = 18 },
    @{ face = 'Georgia'; logicalHeight = 24 }
)
foreach ($dpi in @(96, 120, 144, 192)) {
    $scale = $dpi / 96.0
    foreach ($logicalClientWidth in @(760, 900, 1232)) {
        $clientWidth = [int][Math]::Round($logicalClientWidth * $scale)
        $graphicsWidth = [Math]::Min([int][Math]::Round(600 * $scale),
            [Math]::Max([int][Math]::Round(420 * $scale), $clientWidth / 2 - [int][Math]::Round(32 * $scale)))
        $gap = [int][Math]::Round(8 * $scale)
        $inset = [int][Math]::Round(16 * $scale)
        $previewX = $inset + $graphicsWidth + $gap * 2
        $previewWidth = [Math]::Max([int][Math]::Round(260 * $scale), $clientWidth - $previewX - $inset)
        $previewHalf = [int][Math]::Floor(($previewWidth - $gap) / 2)
        $third = [int][Math]::Floor(($graphicsWidth - $gap * 2) / 3)
        $half = [int][Math]::Floor(($graphicsWidth - $gap) / 2)
        $toggleRowWidth = [int][Math]::Round((104 * 2 + 144) * $scale) + $gap * 2
        Check ($toggleRowWidth -le $graphicsWidth) `
            "Glass/mist/dust toggles remain positive and bounded inside the ${graphicsWidth}px graphics panel at ${dpi} DPI."
        Check ($previewWidth -gt 0 -and $previewHalf -gt 0 -and $previewX + $previewWidth -le $clientWidth - $inset) `
            "Right preview panel remains positive and within the window at ${logicalClientWidth}px client width and ${dpi} DPI."
        $previewRowsHeight = [int][Math]::Round((36 * 3 + 8 * 2) * $scale)
        $previewRowsTop = [int][Math]::Round(521 * $scale) - $inset - $previewRowsHeight
        Check ($previewRowsTop -ge [int][Math]::Round((192 + 76) * $scale)) `
            "Three preview button rows stay below the graph and within the 521-DIP minimum client height at ${dpi} DPI."
        $widths = @{
            preset = $graphicsWidth; fire = $third; shadow = $third; water = $third
            glass = [int][Math]::Round(104 * $scale); mist = [int][Math]::Round(104 * $scale)
            dust = [int][Math]::Round(144 * $scale); apply = $third; confirm = $third
            revert = $third; reset = $half; back = $half
            previewPause = $previewHalf; previewCamera = $previewHalf
            previewMotion = $previewWidth; previewReset = $previewWidth
        }
        foreach ($fontFixture in $fontFixtures) {
            foreach ($fixture in $captionFixtures) {
                $interiorWidth = $widths[$fixture.row] - [int][Math]::Round(30 * $scale)
                Check ((Test-NativeCaptionFits $fixture.text $interiorWidth $dpi `
                    $fontFixture.face $fontFixture.logicalHeight)) `
                    "Full caption '$($fixture.text)' fits its measured button interior at ${logicalClientWidth}px client width and ${dpi} DPI using $($fontFixture.face)."
            }
        }
    }
}

$worstGraphicsInfo = @'
Effective: 100%  |  internal 1920x1080  |  output 1920x1080  |  glass On
Uploaded fire: High  |  shadows: Current  |  mist: On  |  indoor dust: Standard
High optical build: physical panes retained; profile is fixed by this build.
Water: Off omits water, Mobile refracts, High adds scene reflections.
Fire: Low/Mobile/High use 2/4/10 steps; light strength unchanged.
Shadows: Lower fixed centre1; Current area1; Higher area2/4. Cost unmeasured.
Indoor dust: Off/Low/Standard controls bounded visible motes; cost not yet measured.
Apply needs an RT frame. Keep confirms within 15 foreground seconds.
Requested scene could not load. Previous graphics and scene restored; saved settings and pending recovery remain unchanged.
'@.Trim()
$worstPreviewTelemetry = @'
Preview scene performance | cap 30 Hz
Successful RT presents/s 30.0 | loop 25.1 ms | CPU render 7.2 ms | GPU unavailable
Tracked scene allocations: device-local 210 MiB, host-visible 22 MiB (may overlap). Budget/residency unavailable.
Swapchain success rate, not scanout FPS. Preview does not predict full-game sustained performance.
'@.Trim()
foreach ($dpi in @(96, 120, 144, 192)) {
    $scale = $dpi / 96.0
    $clientWidth = [int][Math]::Round(760 * $scale)
    $gap = [int][Math]::Round(8 * $scale)
    $inset = [int][Math]::Round(16 * $scale)
    $graphicsWidth = [Math]::Min([int][Math]::Round(600 * $scale),
        [Math]::Max([int][Math]::Round(420 * $scale), $clientWidth / 2 - [int][Math]::Round(32 * $scale)))
    $previewWidth = [Math]::Max([int][Math]::Round(260 * $scale),
        $clientWidth - ($inset + $graphicsWidth + $gap * 2) - $inset)
    $infoHeight = [int][Math]::Round(148 * $scale)
    $telemetryHeight = [int][Math]::Round(120 * $scale)
    $infoWidth = $graphicsWidth
    Check (Test-NativeStaticTextFits $worstGraphicsInfo $infoWidth $infoHeight $dpi 14) `
        "Full graphics description and longest recovery status wrap inside the info panel at 760-DIP client width and ${dpi} DPI using the actual 14-DIP Segoe UI font."
    Check (Test-NativeStaticTextFits $worstPreviewTelemetry $previewWidth $telemetryHeight $dpi 14) `
        "Full preview telemetry wraps inside the 120-DIP right panel at 760-DIP client width and ${dpi} DPI using the actual 14-DIP Segoe UI font."
}
foreach ($fontFixture in $fontFixtures) {
    Write-Output "GDI font fixture $($fontFixture.face) resolved as $($script:graphicsGdiResolvedFaces[$fontFixture.face])."
}
Write-Output "PASS: Windows glass graphics source contracts; $script:checks checks. No Vulkan/GUI/device execution."
