$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$owner = Get-Content -LiteralPath (Join-Path $root 'src/platform/windows/DiagnosticWindow.cpp') -Raw
$review = Get-Content -LiteralPath (Join-Path $root 'src/platform/windows/WindowsBenchmarkSummaryReview.inl') -Raw
$script:checks = 0
function Check([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
    $script:checks++
}
function Section([string]$text, [string]$begin, [string]$end) {
    $start = $text.IndexOf($begin, [StringComparison]::Ordinal)
    if ($start -lt 0) { throw "Missing source boundary $begin" }
    $stop = $text.IndexOf($end, $start + $begin.Length, [StringComparison]::Ordinal)
    if ($stop -le $start) { throw "Missing source boundary $end" }
    return $text.Substring($start, $stop - $start)
}
function Test-CompletedFreeze([string]$body) {
    $drain = $body.IndexOf('CompleteRtEvidenceAfterDeviceIdle(context, idleResult)')
    $finalize = $body.IndexOf('context.benchmarkEvidence.Finalize()')
    $freeze = $body.IndexOf('FreezeWindowsBenchmarkSummary(context, capabilities)')
    $reset = $body.IndexOf('ResetRoute(context);')
    return $drain -ge 0 -and $drain -lt $finalize -and $finalize -lt $freeze -and $freeze -lt $reset
}
function Test-ExactOwnerFreeze([string]$body) {
    return $body.Contains('!context.benchmark.Passed()') -and
        $body.Contains('context.benchmarkEvidence.Status() != horde::telemetry::RtBenchmarkRunStatus::Complete') -and
        $body.Contains('context.rtScene.Profile() != arm.profile') -and
        $body.Contains('CurrentGraphicsSettings(context) == arm.applied') -and
        $body.Contains('context.renderScaleDirty || context.sceneProfileDirty || context.glassGeometryDirty') -and
        $body.Contains('arm.configuration, completion, runUuid') -and
        $body.Contains('if (frozen.IsReady()) context.benchmarkSummary.emplace') -and
        $body.Contains('catch (...) { context.benchmarkSummary.reset(); }')
}
$complete = Section $owner 'void CompleteBenchmark(' 'void ToggleFullscreen('
$start = Section $owner 'void StartBenchmark(' 'void LogBenchmarkCancellation('
$cancel = Section $owner 'void CancelBenchmark(' 'void CompleteBenchmark('
$arm = Section $review 'void ArmWindowsBenchmarkSummary(' 'void FreezeWindowsBenchmarkSummary('
$freeze = Section $review 'void FreezeWindowsBenchmarkSummary(' 'constexpr wchar_t kSummaryReviewWindowClass'
$configuration = Section $review 'horde::telemetry::BenchmarkSummaryConfiguration WindowsBenchmarkSummaryConfiguration(' 'void ArmWindowsBenchmarkSummary('
$prepare = Section $review 'void PrepareWindowsSummaryReview(' 'LRESULT CALLBACK WindowsSummaryReviewWindowProc('
$controls = Section $review 'bool CreateWindowsSummaryReviewControls(' 'void PrepareWindowsSummaryReview('
$windowProc = Section $review 'LRESULT CALLBACK WindowsSummaryReviewWindowProc(' 'void ShowWindowsBenchmarkSummaryReview('
$clipboard = Section $review 'bool CopyWindowsSummaryJson(' 'int SummaryReviewScale('
$utf8 = Section $review 'bool WindowsSummaryUtf8Preview(' 'bool CopyWindowsSummaryJson('
$show = $review.Substring($review.IndexOf('void ShowWindowsBenchmarkSummaryReview('))
Check (Test-CompletedFreeze $complete) 'Optional frozen statistics must follow the final owning completion/finalize and precede route reset.'
Check (-not (Test-CompletedFreeze ($complete.Replace('FreezeWindowsBenchmarkSummary(context, capabilities)', 'deferCaptureUntilAfterReset')))) 'Negative source fixture rejects missing before-reset ownership capture.'
Check (-not $complete.Contains('ShowWindowsBenchmarkSummaryReview(') -and
    -not $complete.Contains('CopyWindowsSummaryJson(') -and -not $complete.Contains('ExportWindowsPlaytestReportJson(')) 'Completion cannot automatically open, copy or export a statistics review.'
Check ($start.IndexOf('context.benchmarkSummary.reset()') -lt $start.IndexOf('ResetRoute(context)') -and
    $cancel.Contains('context.benchmarkSummaryArm.reset()') -and $cancel.Contains('context.benchmarkSummary.reset()')) 'Accepted new runs and cancellation invalidate prior summary owners.'
Check ($review.Contains('!context.unattendedBenchmark && !context.graphicsPreviewCapture') -and
    $review.Contains('!context.outputResizeValidation && !context.nativeMotionValidation') -and
    $arm.Contains('if (!WindowsBenchmarkSummaryInteractive(context)) return') -and
    $freeze.Contains('!WindowsBenchmarkSummaryInteractive(context)')) 'Unattended observer/capture modes perform no optional summary snapshot or UI work.'
Check ($owner.IndexOf('ArmWindowsBenchmarkSummary(context)') -gt $owner.IndexOf('context.benchmarkEvidence.ArmMeasurement(state.sceneEpoch, state.measurementGeneration)') -and
    $arm.Contains('RtBenchmarkRunStatus::Measuring') -and $arm.Contains('arm.configuration.sceneEpoch != context.benchmarkEvidence.SceneEpoch()') -and
    $arm.Contains('arm.configuration.measurementGeneration != context.benchmarkEvidence.MeasurementGeneration()')) 'The arming snapshot must name the real successful measured scope rather than warm-up seeds.'
Check ($arm.Contains('Its cost remains inside the existing timing scope') -and
    $arm.Contains('catch (...) { context.benchmarkSummaryArm.reset(); }') -and
    -not $arm.Contains('CaptureBenchmarkSummary(') -and -not $arm.Contains('MakeWindowsBenchmarkUuid(')) 'Only bounded metadata is copied on the interactive arm frame; optional heavy capture/identity work occurs after completion and failures are nonfatal.'
Check ($configuration.Contains('configuration.metadata = BuildBenchmarkMetadata(context, capabilities)') -and
    $configuration.Contains('CurrentGraphicsSettings(context)') -and
    $configuration.Contains('configuration.glassEnabled = context.rtScene.GlassEnabled()') -and
    $configuration.Contains('configuration.sceneEpoch = state.sceneEpoch') -and
    $configuration.Contains('configuration.measurementGeneration = state.measurementGeneration')) 'Configuration records actual tuple, selected pair/backend/extents and owner seeds.'
Check (Test-ExactOwnerFreeze $freeze) 'Completed owner freezes only a successful unchanged physical configuration with strict shared population validation.'
Check (-not (Test-ExactOwnerFreeze ($freeze.Replace('CurrentGraphicsSettings(context) == arm.applied', 'trustCurrentWithoutCheckingArmedTuple')))) 'Negative source fixture rejects configuration changes including the fifth-field cap/glass tuple.'
Check (-not (Test-ExactOwnerFreeze ($freeze.Replace('context.benchmarkEvidence.Status() != horde::telemetry::RtBenchmarkRunStatus::Complete', 'acceptIncompleteEvidence')))) 'Negative source fixture rejects incomplete owner evidence.'
Check ($freeze.Contains('MakeWindowsBenchmarkUuid(runUuid)') -and $freeze.Contains('BenchmarkSummaryCooling::Unknown') -and
    $freeze.Contains('runUuid, "Unknown"')) 'Completed run identity is independently random; cooling and unavailable PC model are explicitly Unknown.'
Check ($review.Contains('BCryptGenRandom(nullptr') -and $review.Contains('BCRYPT_USE_SYSTEM_PREFERRED_RNG') -and
    $review.Contains('random[6u] & 0x0fu) | 0x40u') -and $review.Contains('random[8u] & 0x3fu) | 0x80u') -and
    $review.Contains('IsBenchmarkSummaryUuid(id)')) 'Run and report identity helper emits CSPRNG UUIDv4 with required version/variant, independently of existing schema1 hex IDs.'
Check ($prepare.Contains('MakeWindowsBenchmarkUuid(reportUuid)') -and
    $prepare.Contains('reportUuid == state.frozen.Data().runUuid')) 'Prepared report identity is separate from the completed run identity.'
Check ($controls.Contains('kSummaryConsent), BM_SETCHECK, BST_UNCHECKED') -and
    $controls.Contains('kSummaryHardware), BM_SETCHECK, BST_UNCHECKED') -and
    $controls.Contains('SendMessageW(cooling, CB_SETCURSEL, 0, 0)')) 'Every new form starts prepare consent OFF, hardware OFF, cooling Unknown.'
Check ($controls.Contains('for (const int id : {kSummaryPrepare, kSummaryCopy, kSummarySave}) EnableWindow(SummaryReviewControl(window, id), FALSE)') -and
    $controls.Contains('ES_READONLY | ES_MULTILINE')) 'Before consent/preparation, local actions are disabled and the exact preview is read-only.'
Check ($prepare.Contains('state.prepared.IsReady() || !state.frozen.IsReady()') -and
    $prepare.Contains('BM_GETCHECK, 0, 0) != BST_CHECKED') -and
    $prepare.Contains('PrepareBenchmarkSummaryReport(state.frozen,') -and
    $prepare.Contains('WindowsSummaryUtf8Preview(prepared.Json(), preview)')) 'Prepare requires explicit consent and the immutable typed owner, then validates exact UTF-8 before enabling actions.'
Check ($prepare.Contains('state.prepared = std::move(prepared); state.preview = std::move(preview)') -and
    $prepare.Contains('{kSummaryConsent, kSummaryHardware, kSummaryCooling, kSummaryPrepare}') -and
    -not $prepare.Contains('ExportWindowsPlaytestReportJson') -and -not $prepare.Contains('CopyWindowsSummaryJson')) 'Preparation freezes choices without copying or saving.'
Check ($utf8.Contains('kBenchmarkSummaryReportMaxBytes') -and $utf8.Contains('CP_UTF8, MB_ERR_INVALID_CHARS') -and
    $utf8.Contains('if (count <= 0) return false')) 'Preview rejects oversized or invalid UTF-8 rather than replacing characters.'
Check ($clipboard.Contains('!state.prepared.IsReady()') -and $clipboard.Contains('state.prepared.Json()') -and
    $clipboard.Contains('CF_UNICODETEXT') -and $clipboard.Contains('RegisterClipboardFormatW(L"application/json")') -and
    $clipboard.Contains('std::memcpy(bytes, json.data(), json.size())') -and -not $clipboard.Contains('CF_TEXT')) 'Copy uses the frozen JSON bytes plus lossless Unicode text, with no ANSI conversion or reserialization.'
Check ($clipboard.Contains('if (!text) { release(); return false; }') -and
    $clipboard.Contains('if (!bytes) { release(); return false; }') -and
    $clipboard.Contains('CloseClipboard(); release(); return copied')) 'Clipboard allocation/lock/publication failures retain report bytes and release only unowned handles.'
Check ($windowProc.Contains('case kSummarySave:') -and $windowProc.Contains('state->prepared.IsReady() && !state->saved') -and
    $windowProc.Contains('const std::string exactJson(state->prepared.Json())') -and
    $windowProc.Contains('ExportWindowsPlaytestReportJson(window, exactJson, status)')) 'Only explicit Save invokes the existing checked local UTF-8 picker with frozen exact JSON; retries reuse the same report.'
Check ($windowProc.Contains('catch (...)') -and $show.Contains('std::unique_ptr<WindowsBenchmarkSummaryReviewState> state;') -and
    $show.IndexOf('std::unique_ptr<WindowsBenchmarkSummaryReviewState> state;') -lt $show.IndexOf("`n    try") -and
    $show.Contains('state->frozen = frozen') -and $show.Contains('state.get()')) 'Form owns a copied immutable summary and retains callback state through exceptional cleanup.'
Check ($show.Contains('EnableWindow(owner, FALSE)') -and $show.Contains('while (IsWindow(window) && IsWindow(owner))') -and
    $show.Contains('if (IsWindow(window)) DestroyWindow(window)') -and $show.Contains('if (IsWindow(owner) && ownerEnabled)') -and
    $show.Contains('PostQuitMessage(static_cast<int>(message.wParam))')) 'Modal review blocks owner configuration/start actions and safely closes on owner destruction/quit without losing WM_QUIT or enabling an originally disabled owner.'
Check ($review.Contains('WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS') -and
    $review.Contains('id == kSummaryCopy || id == kSummarySave || id == kSummaryClose ? window : body') -and
    $review.Contains('contentHeight - viewport') -and $review.Contains('y - state->scrollOffset') -and
    $windowProc.Contains('message == WM_VSCROLL || message == WM_MOUSEWHEEL') -and
    $windowProc.Contains('monitor.rcWork.bottom - monitor.rcWork.top')) 'Large-DPI review body scrolls within a clipped work-area-bounded panel; local action footer remains visible.'
Check ($review.Contains('RevealWindowsSummaryReviewFocus(') -and $review.Contains('GetParent(control) != body') -and
    $windowProc.Contains('if (notification != BN_CLICKED) return 0')) 'Keyboard focus reveals offscreen body fields and cannot implicitly Prepare/Copy/Save.'
foreach ($forbidden in @('WinHttp', 'SendPlaytest', 'ShowWindowsRemote', 'WindowsPlaytestSubmission', 'ShellExecute',
    'HttpClient', 'SettingsPath(', 'WriteReportFile(', 'benchmarkJsonReport', 'simulation.')) {
    Check (-not $review.Contains($forbidden)) "Local review has no unrelated transport, settings, mutable report-file or gameplay boundary: $forbidden"
}
$button = Section $owner 'case kBenchmarkReviewStatsButtonId:' 'case kBenchmarkCopyButtonId:'
Check ($button.Contains('!sceneContext->benchmark.IsRunning()') -and $button.Contains('benchmarkSummary->IsReady()') -and
    $button.Contains('ShowWindowsBenchmarkSummaryReview(hWnd, *sceneContext->benchmarkSummary)')) 'Only explicit enabled completed-summary button opens the modal form.'
$roster = Section $owner 'std::vector<HWND> VisibleControllerMenuControls(' 'void NavigateControllerMenu('
Check ($roster.Contains('std::to_array<int>') -and $roster.Contains('kBenchmarkReviewStatsButtonId')) 'New native review action participates in deduced-size controller navigation without a manual roster-capacity regression.'
Write-Output "PASS: Windows benchmark summary review source contracts; $script:checks checks. No GUI, clipboard, picker, GPU or device execution."
