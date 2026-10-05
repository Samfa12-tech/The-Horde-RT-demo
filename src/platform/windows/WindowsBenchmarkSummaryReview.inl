// Included in the ordinary Windows owner, independently of Debug capture modes.
// The existing schema1 playtest identity helper remains unchanged. This UUIDv4
// helper is shared only by the new local benchmark run and report identities.
bool MakeWindowsBenchmarkUuid(std::string& id)
{
    std::array<unsigned char, 16u> random{};
    if (BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) return false;
    random[6u] = static_cast<unsigned char>((random[6u] & 0x0fu) | 0x40u);
    random[8u] = static_cast<unsigned char>((random[8u] & 0x3fu) | 0x80u);
    constexpr char hex[] = "0123456789abcdef";
    id.clear(); id.reserve(36u);
    for (std::size_t index = 0u; index < random.size(); ++index)
    {
        if (index == 4u || index == 6u || index == 8u || index == 10u) id.push_back('-');
        id.push_back(hex[random[index] >> 4u]); id.push_back(hex[random[index] & 0x0fu]);
    }
    return horde::telemetry::IsBenchmarkSummaryUuid(id);
}

bool WindowsBenchmarkSummaryInteractive(const VulkanSurfaceContext& context) noexcept
{
    return !context.unattendedBenchmark && !context.graphicsPreviewCapture &&
        !context.outputResizeValidation && !context.nativeMotionValidation;
}

horde::telemetry::BenchmarkSummaryConfiguration WindowsBenchmarkSummaryConfiguration(
    const VulkanSurfaceContext& context, const horde::vulkan::DeviceCapabilities& capabilities)
{
    horde::telemetry::BenchmarkSummaryConfiguration configuration;
    configuration.platform = horde::telemetry::BenchmarkSummaryPlatform::Windows;
    configuration.metadata = BuildBenchmarkMetadata(context, capabilities);
    const auto applied = CurrentGraphicsSettings(context);
    configuration.metadata.renderScalePercent = static_cast<std::uint32_t>(applied.renderScalePercent);
    configuration.water = static_cast<horde::telemetry::RtWaterQuality>(applied.waterQuality);
    if (!context.rtScene.IsReady() || !context.rtScene.HasUploadedQualityControls()) return {};
    switch (context.rtScene.UploadedFireQuality())
    {
    case horde::vulkan::raytracing::FireEmitterQuality::Mobile:
        configuration.fire = horde::telemetry::BenchmarkSummaryFireQuality::Mobile; break;
    case horde::vulkan::raytracing::FireEmitterQuality::High:
        configuration.fire = horde::telemetry::BenchmarkSummaryFireQuality::High; break;
    case horde::vulkan::raytracing::FireEmitterQuality::Low:
        configuration.fire = horde::telemetry::BenchmarkSummaryFireQuality::Low; break;
    default: return {};
    }
    const auto fireBudget = horde::vulkan::raytracing::ResolveFireEmitterQualityBudget(context.rtScene.UploadedFireQuality());
    configuration.uploadedFireQuality = horde::telemetry::RtFireQualityEvidence{
        static_cast<horde::telemetry::RtFireQuality>(configuration.fire), fireBudget.volumeSteps, fireBudget.reflectionSamples};
    const auto& quality = context.rtScene.QualityControls().controls;
    configuration.shadowQuality = horde::telemetry::RtShadowQualityEvidence{
        static_cast<horde::telemetry::RtShadowMode>(quality[0]), quality[1], quality[2], 0u};
    configuration.actualUploadedMistEnabled = context.rtScene.UploadedMistEnabled();
    if (!configuration.actualUploadedMistEnabled) return {};
    const auto optical = context.rtScene.SelectedDielectricQualityName();
    if (optical != "High" && optical != "Mobile") return {};
    configuration.dielectric = optical == "High" ?
        horde::telemetry::RtDielectricQuality::High : horde::telemetry::RtDielectricQuality::Mobile;
    configuration.glassEnabled = context.rtScene.GlassEnabled();
    const auto state = context.rtFrameEvidence.PublishedStateByValue();
    configuration.sceneEpoch = state.sceneEpoch;
    configuration.measurementGeneration = state.measurementGeneration;
    return configuration;
}

void ArmWindowsBenchmarkSummary(VulkanSurfaceContext& context) noexcept
{
    if (!WindowsBenchmarkSummaryInteractive(context)) return;
    context.benchmarkSummaryArm.reset();
    // One bounded OPTIONAL metadata copy occurs on the interactive arm frame.
    // Its cost remains inside the existing timing scope; no per-frame metadata
    // copies or additional unattended observer work are introduced.
    try
    {
        if (!context.capabilitySnapshot || !context.rtScene.IsReady() || !context.rtScene.HasUploadedQualityControls() ||
            context.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::Showcase ||
            context.graphicsCommand || context.renderScaleDirty || context.sceneProfileDirty || context.glassGeometryDirty ||
            context.benchmarkEvidence.Status() != horde::telemetry::RtBenchmarkRunStatus::Measuring) return;
        WindowsBenchmarkSummaryArm arm;
        arm.configuration = WindowsBenchmarkSummaryConfiguration(context, *context.capabilitySnapshot);
        arm.applied = CurrentGraphicsSettings(context); arm.profile = context.rtScene.Profile();
        if (arm.configuration.sceneEpoch != context.benchmarkEvidence.SceneEpoch() ||
            arm.configuration.measurementGeneration != context.benchmarkEvidence.MeasurementGeneration()) return;
        context.benchmarkSummaryArm.emplace(std::move(arm));
    }
    catch (...) { context.benchmarkSummaryArm.reset(); } // Optional allocation is never a benchmark failure.
}

void FreezeWindowsBenchmarkSummary(VulkanSurfaceContext& context,
                                  const horde::vulkan::DeviceCapabilities& capabilities) noexcept
{
    context.benchmarkSummary.reset();
    if (!WindowsBenchmarkSummaryInteractive(context) || !context.benchmarkSummaryArm) return;
    try
    {
        const auto& arm = *context.benchmarkSummaryArm;
        if (!context.benchmark.Passed() || context.benchmarkEvidence.Status() != horde::telemetry::RtBenchmarkRunStatus::Complete ||
            !context.rtScene.IsReady() || context.rtScene.Profile() != arm.profile ||
            !(CurrentGraphicsSettings(context) == arm.applied) || context.graphicsCommand ||
            context.renderScaleDirty || context.sceneProfileDirty || context.glassGeometryDirty) return;
        const auto completion = WindowsBenchmarkSummaryConfiguration(context, capabilities);
        std::string runUuid;
        if (!MakeWindowsBenchmarkUuid(runUuid)) return;
        auto frozen = horde::telemetry::CaptureBenchmarkSummary(context.benchmark, context.benchmarkEvidence,
            arm.configuration, completion, runUuid, "Unknown", horde::telemetry::BenchmarkSummaryCooling::Unknown);
        if (frozen.IsReady()) context.benchmarkSummary.emplace(std::move(frozen));
    }
    catch (...) { context.benchmarkSummary.reset(); } // Legacy reports and outcome remain authoritative.
}

constexpr wchar_t kSummaryReviewWindowClass[] = L"HordeLanternBenchmarkSummaryReview";
constexpr int kSummaryIntro = 100, kSummaryConsent = 101, kSummaryHardware = 102;
constexpr int kSummaryCoolingLabel = 103, kSummaryCooling = 104, kSummaryPrepare = 105;
constexpr int kSummaryPreviewLabel = 106, kSummaryPreview = 107, kSummaryStatus = 108;
constexpr int kSummaryCopy = 109, kSummarySave = 110, kSummaryClose = IDCANCEL;
constexpr int kSummaryBodyPanel = 111;
struct WindowsBenchmarkSummaryReviewState
{
    // Owned copy: no render-owner pointers, mutable report files or post-reset state.
    horde::telemetry::FrozenBenchmarkSummary frozen;
    horde::reporting::PreparedBenchmarkSummaryReport prepared;
    std::wstring preview;
    HFONT font = nullptr;
    bool saved = false;
    int scrollOffset = 0;
};

HWND SummaryReviewControl(HWND window, const int id)
{
    if (id == kSummaryCopy || id == kSummarySave || id == kSummaryClose || id == kSummaryBodyPanel)
        return GetDlgItem(window, id);
    return GetDlgItem(GetDlgItem(window, kSummaryBodyPanel), id);
}

bool WindowsSummaryUtf8Preview(const std::string_view json, std::wstring& wide)
{
    if (json.empty() || json.size() > horde::reporting::kBenchmarkSummaryReportMaxBytes) return false;
    const int bytes = static_cast<int>(json.size());
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, json.data(), bytes, nullptr, 0);
    if (count <= 0) return false;
    wide.resize(static_cast<std::size_t>(count));
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, json.data(), bytes, wide.data(), count) == count;
}

bool CopyWindowsSummaryJson(HWND window, const WindowsBenchmarkSummaryReviewState& state)
{
    if (!state.prepared.IsReady() || state.preview.empty()) return false;
    const auto json = state.prepared.Json();
    const UINT utf8Format = RegisterClipboardFormatW(L"application/json");
    if (!utf8Format) return false;
    HGLOBAL textMemory = GlobalAlloc(GMEM_MOVEABLE, (state.preview.size() + 1u) * sizeof(wchar_t));
    HGLOBAL utf8Memory = GlobalAlloc(GMEM_MOVEABLE, json.size() + 1u);
    const auto release = [&]() { if (textMemory) GlobalFree(textMemory); if (utf8Memory) GlobalFree(utf8Memory); };
    if (!textMemory || !utf8Memory) { release(); return false; }
    void* text = GlobalLock(textMemory);
    if (!text) { release(); return false; }
    std::memcpy(text, state.preview.c_str(), (state.preview.size() + 1u) * sizeof(wchar_t));
    GlobalUnlock(textMemory);
    void* bytes = GlobalLock(utf8Memory);
    if (!bytes) { release(); return false; }
    std::memcpy(bytes, json.data(), json.size()); static_cast<char*>(bytes)[json.size()] = '\0';
    GlobalUnlock(utf8Memory);
    if (!OpenClipboard(window)) { release(); return false; }
    bool copied = false;
    if (EmptyClipboard())
    {
        if (SetClipboardData(utf8Format, utf8Memory))
        {
            utf8Memory = nullptr; // Clipboard owns the exact UTF-8 JSON plus its text terminator.
            if (SetClipboardData(CF_UNICODETEXT, textMemory)) { textMemory = nullptr; copied = true; }
        }
    }
    CloseClipboard(); release(); return copied;
}

int SummaryReviewScale(HWND window, int logical)
{
    const UINT dpi = GetDpiForWindow(window);
    return MulDiv(logical, static_cast<int>(dpi ? dpi : 96u), 96);
}
void SetSummaryReviewStatus(HWND window, const wchar_t* text)
{ SetWindowTextW(SummaryReviewControl(window, kSummaryStatus), text); }

void LayoutWindowsSummaryReview(HWND window)
{
    RECT client{}; GetClientRect(window, &client);
    const int margin = SummaryReviewScale(window, 16), gap = SummaryReviewScale(window, 8);
    const int width = std::max(SummaryReviewScale(window, 100), static_cast<int>(client.right) - 2 * margin);
    const int line = SummaryReviewScale(window, 28), button = SummaryReviewScale(window, 36);
    const int buttonY = client.bottom - margin - button, buttonWidth = (width - gap * 2) / 3;
    const int viewport = std::max(1, buttonY - gap - margin);
    MoveWindow(SummaryReviewControl(window, kSummaryBodyPanel), margin, margin, width, viewport, TRUE);
    const int header = SummaryReviewScale(window, 72) + gap + 4 * line + 3 * gap + button + gap + line;
    const int previewHeight = std::max(SummaryReviewScale(window, 100), viewport - header - gap - 2 * line);
    const int contentHeight = header + previewHeight + gap + 2 * line;
    auto* state = reinterpret_cast<WindowsBenchmarkSummaryReviewState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (!state) return;
    state->scrollOffset = std::clamp(state->scrollOffset, 0, std::max(0, contentHeight - viewport));
    SCROLLINFO scroll{sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS};
    scroll.nMin = 0; scroll.nMax = contentHeight - 1; scroll.nPage = static_cast<UINT>(viewport); scroll.nPos = state->scrollOffset;
    SetScrollInfo(window, SB_VERT, &scroll, TRUE);
    const auto place = [&](int id, int y, int height) {
        MoveWindow(SummaryReviewControl(window, id), 0, y - state->scrollOffset, width, height, TRUE);
    };
    int y = 0;
    place(kSummaryIntro, y, SummaryReviewScale(window, 72)); y += SummaryReviewScale(window, 72) + gap;
    place(kSummaryConsent, y, line); y += line + gap;
    place(kSummaryHardware, y, line); y += line + gap;
    place(kSummaryCoolingLabel, y, line); y += line;
    place(kSummaryCooling, y, line + SummaryReviewScale(window, 100)); y += line + gap;
    place(kSummaryPrepare, y, button); y += button + gap;
    place(kSummaryPreviewLabel, y, line); y += line;
    place(kSummaryPreview, y, previewHeight); y += previewHeight + gap;
    place(kSummaryStatus, y, line * 2);
    int x = margin;
    for (const int id : {kSummaryCopy, kSummarySave, kSummaryClose})
    { MoveWindow(SummaryReviewControl(window, id), x, buttonY, buttonWidth, button, TRUE); x += buttonWidth + gap; }
}

void RevealWindowsSummaryReviewFocus(HWND window, HWND control, WindowsBenchmarkSummaryReviewState& state)
{
    HWND body = SummaryReviewControl(window, kSummaryBodyPanel);
    if (!control || GetParent(control) != body) return;
    RECT bounds{}, viewport{};
    GetWindowRect(control, &bounds); GetClientRect(body, &viewport);
    MapWindowPoints(HWND_DESKTOP, body, reinterpret_cast<POINT*>(&bounds), 2);
    if (GetDlgCtrlID(control) == kSummaryCooling) bounds.bottom = bounds.top + SummaryReviewScale(window, 28);
    if (bounds.top < 0) state.scrollOffset += bounds.top;
    else if (bounds.bottom > viewport.bottom) state.scrollOffset += bounds.bottom - viewport.bottom;
    else return;
    LayoutWindowsSummaryReview(window);
}

LRESULT CALLBACK WindowsSummaryReviewBodyProc(HWND panel, UINT message, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR, DWORD_PTR)
{
    if (message == WM_COMMAND || message == WM_NOTIFY || message == WM_MOUSEWHEEL)
        return SendMessageW(GetParent(panel), message, wParam, lParam);
    if (message == WM_NCDESTROY) RemoveWindowSubclass(panel, WindowsSummaryReviewBodyProc, 1u);
    return DefSubclassProc(panel, message, wParam, lParam);
}

void ApplyWindowsSummaryReviewFont(HWND window, WindowsBenchmarkSummaryReviewState& state)
{
    HFONT font = CreateFontW(SummaryReviewScale(window, 17), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT effective = font ? font : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    EnumChildWindows(window, [](HWND child, LPARAM value) -> BOOL {
        SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(value), TRUE); return TRUE;
    }, reinterpret_cast<LPARAM>(effective));
    if (state.font) DeleteObject(state.font);
    state.font = font;
}

bool CreateWindowsSummaryReviewControls(HWND window, WindowsBenchmarkSummaryReviewState& state)
{
    HWND body = CreateWindowExW(WS_EX_CONTROLPARENT, L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 10, 10, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSummaryBodyPanel)), GetModuleHandleW(nullptr), nullptr);
    if (!body || !SetWindowSubclass(body, WindowsSummaryReviewBodyProc, 1u, 0u)) return false;
    const auto add = [&](const wchar_t* kind, const wchar_t* text, DWORD style, int id, DWORD extra = 0u) {
        const HWND parent = id == kSummaryCopy || id == kSummarySave || id == kSummaryClose ? window : body;
        if (std::wstring_view(kind) == L"BUTTON") style |= BS_NOTIFY;
        return CreateWindowExW(extra, kind, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, parent,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr) != nullptr;
    };
    if (!add(L"STATIC", L"Review a completed benchmark as local JSON. Nothing is sent. Prepare consent and hardware inclusion start OFF. Optional hardware contains only the Vulkan GPU/API; PC model is Unknown.", SS_LEFT | SS_NOPREFIX, kSummaryIntro) ||
        !add(L"BUTTON", L"I consent to prepare this local statistics report", BS_AUTOCHECKBOX | WS_TABSTOP, kSummaryConsent) ||
        !add(L"BUTTON", L"Include basic hardware labels (optional)", BS_AUTOCHECKBOX | WS_TABSTOP, kSummaryHardware) ||
        !add(L"STATIC", L"Cooling declaration (optional; not a sensor measurement)", SS_LEFT | SS_NOPREFIX, kSummaryCoolingLabel) ||
        !add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, kSummaryCooling) ||
        !add(L"BUTTON", L"Prepare JSON for review", BS_PUSHBUTTON | WS_TABSTOP, kSummaryPrepare) ||
        !add(L"STATIC", L"Exact prepared JSON (read-only)", SS_LEFT, kSummaryPreviewLabel) ||
        !add(L"EDIT", L"", ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP, kSummaryPreview, WS_EX_CLIENTEDGE) ||
        !add(L"STATIC", L"Choose consent, then Prepare. Copy and Save remain separate explicit actions.", SS_LEFT | SS_NOPREFIX, kSummaryStatus) ||
        !add(L"BUTTON", L"Copy JSON", BS_PUSHBUTTON | WS_TABSTOP, kSummaryCopy) ||
        !add(L"BUTTON", L"Save JSON...", BS_PUSHBUTTON | WS_TABSTOP, kSummarySave) ||
        !add(L"BUTTON", L"Close", BS_PUSHBUTTON | WS_TABSTOP, kSummaryClose)) return false;
    SendMessageW(SummaryReviewControl(window, kSummaryConsent), BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessageW(SummaryReviewControl(window, kSummaryHardware), BM_SETCHECK, BST_UNCHECKED, 0);
    HWND cooling = SummaryReviewControl(window, kSummaryCooling);
    for (const wchar_t* choice : {L"Unknown / not declared", L"No external cooling declared", L"External cooling declared"})
        SendMessageW(cooling, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(choice));
    SendMessageW(cooling, CB_SETCURSEL, 0, 0);
    for (const int id : {kSummaryPrepare, kSummaryCopy, kSummarySave}) EnableWindow(SummaryReviewControl(window, id), FALSE);
    ApplyWindowsSummaryReviewFont(window, state); LayoutWindowsSummaryReview(window); return true;
}

void PrepareWindowsSummaryReview(HWND window, WindowsBenchmarkSummaryReviewState& state)
{
    if (state.prepared.IsReady() || !state.frozen.IsReady() ||
        SendMessageW(SummaryReviewControl(window, kSummaryConsent), BM_GETCHECK, 0, 0) != BST_CHECKED) return;
    const bool hardware = SendMessageW(SummaryReviewControl(window, kSummaryHardware), BM_GETCHECK, 0, 0) == BST_CHECKED;
    const LRESULT cooling = SendMessageW(SummaryReviewControl(window, kSummaryCooling), CB_GETCURSEL, 0, 0);
    if (cooling < 0 || cooling > 2) { SetSummaryReviewStatus(window, L"Choose a valid cooling declaration."); return; }
    std::string reportUuid;
    if (!MakeWindowsBenchmarkUuid(reportUuid) || reportUuid == state.frozen.Data().runUuid)
    { SetSummaryReviewStatus(window, L"Could not create a distinct secure report ID. Nothing was prepared."); return; }
    const auto timestamp = UtcTimestamp("%Y-%m-%dT%H:%M:%SZ");
    auto prepared = horde::reporting::PrepareBenchmarkSummaryReport(state.frozen,
        {true, hardware, reportUuid, timestamp, static_cast<horde::telemetry::BenchmarkSummaryCooling>(cooling)});
    std::wstring preview;
    if (!prepared.IsReady() || !WindowsSummaryUtf8Preview(prepared.Json(), preview))
    { SetSummaryReviewStatus(window, L"The bounded statistics report could not be prepared. Nothing was copied or saved."); return; }
    state.prepared = std::move(prepared); state.preview = std::move(preview);
    SetWindowTextW(SummaryReviewControl(window, kSummaryPreview), state.preview.c_str());
    for (const int id : {kSummaryConsent, kSummaryHardware, kSummaryCooling, kSummaryPrepare}) EnableWindow(SummaryReviewControl(window, id), FALSE);
    for (const int id : {kSummaryCopy, kSummarySave}) EnableWindow(SummaryReviewControl(window, id), TRUE);
    SetSummaryReviewStatus(window, L"Review these fixed bytes. Copy or Save only when you choose; nothing is sent.");
}

LRESULT CALLBACK WindowsSummaryReviewWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* state = reinterpret_cast<WindowsBenchmarkSummaryReviewState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return DefWindowProcW(window, message, wParam, lParam);
    }
    try
    {
        if (message == WM_CREATE) return state && CreateWindowsSummaryReviewControls(window, *state) ? 0 : -1;
        if (message == WM_SIZE) { LayoutWindowsSummaryReview(window); return 0; }
        if (message == WM_DPICHANGED && state)
        {
            const auto* rect = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(window, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            ApplyWindowsSummaryReviewFont(window, *state); LayoutWindowsSummaryReview(window); return 0;
        }
        if (message == WM_GETMINMAXINFO)
        {
            auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
            MONITORINFO monitor{sizeof(MONITORINFO)};
            if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
                limits->ptMinTrackSize = {
                    std::min(SummaryReviewScale(window, 640), static_cast<int>(monitor.rcWork.right - monitor.rcWork.left)),
                    std::min(SummaryReviewScale(window, 680), static_cast<int>(monitor.rcWork.bottom - monitor.rcWork.top))};
            return 0;
        }
        if ((message == WM_VSCROLL || message == WM_MOUSEWHEEL) && state)
        {
            if (message == WM_MOUSEWHEEL)
                state->scrollOffset -= GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * SummaryReviewScale(window, 48);
            else
            {
                SCROLLINFO scroll{sizeof(SCROLLINFO), SIF_ALL}; GetScrollInfo(window, SB_VERT, &scroll);
                switch (LOWORD(wParam))
                {
                case SB_LINEUP: state->scrollOffset -= SummaryReviewScale(window, 24); break;
                case SB_LINEDOWN: state->scrollOffset += SummaryReviewScale(window, 24); break;
                case SB_PAGEUP: state->scrollOffset -= static_cast<int>(scroll.nPage); break;
                case SB_PAGEDOWN: state->scrollOffset += static_cast<int>(scroll.nPage); break;
                case SB_THUMBTRACK: case SB_THUMBPOSITION: state->scrollOffset = scroll.nTrackPos; break;
                case SB_TOP: state->scrollOffset = 0; break;
                case SB_BOTTOM: state->scrollOffset = scroll.nMax; break;
                }
            }
            LayoutWindowsSummaryReview(window); return 0;
        }
        if (message == WM_COMMAND && state)
        {
            const int notification = HIWORD(wParam);
            if (notification == BN_SETFOCUS || notification == EN_SETFOCUS || notification == CBN_SETFOCUS)
                RevealWindowsSummaryReviewFocus(window, reinterpret_cast<HWND>(lParam), *state);
            if (notification != BN_CLICKED) return 0; // Focus/selection is never consent, Prepare, Copy or Save.
            switch (LOWORD(wParam))
            {
            case kSummaryConsent:
                if (!state->prepared.IsReady()) EnableWindow(SummaryReviewControl(window, kSummaryPrepare),
                    SendMessageW(SummaryReviewControl(window, kSummaryConsent), BM_GETCHECK, 0, 0) == BST_CHECKED);
                return 0;
            case kSummaryPrepare: PrepareWindowsSummaryReview(window, *state); return 0;
            case kSummaryCopy:
                SetSummaryReviewStatus(window, CopyWindowsSummaryJson(window, *state) ?
                    L"Prepared JSON copied as Unicode text and exact UTF-8 application/json. Nothing was sent." :
                    L"Clipboard copy failed or was partial. Retry explicitly with the same prepared report."); return 0;
            case kSummarySave:
                if (state->prepared.IsReady() && !state->saved)
                {
                    // Reuse the existing foreground picker and checked binary UTF-8 write.
                    // Its generic playtest filename can be renamed in the picker.
                    const std::string exactJson(state->prepared.Json());
                    std::wstring status;
                    state->saved = horde::platform::windows::ExportWindowsPlaytestReportJson(window, exactJson, status);
                    SetSummaryReviewStatus(window, status.c_str());
                    if (state->saved) EnableWindow(SummaryReviewControl(window, kSummarySave), FALSE);
                }
                return 0;
            case kSummaryClose: DestroyWindow(window); return 0;
            }
        }
        if (message == WM_CLOSE) { DestroyWindow(window); return 0; }
        if (message == WM_NCDESTROY && state)
        {
            if (state->font) { DeleteObject(state->font); state->font = nullptr; }
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        }
    }
    catch (...) { SetSummaryReviewStatus(window, L"Local review could not complete this action. No report was sent; close or retry explicitly."); }
    return DefWindowProcW(window, message, wParam, lParam);
}

void ShowWindowsBenchmarkSummaryReview(HWND owner, const horde::telemetry::FrozenBenchmarkSummary& frozen) noexcept
{
    if (!frozen.IsReady() || !IsWindow(owner)) return;
    HWND window = nullptr;
    std::unique_ptr<WindowsBenchmarkSummaryReviewState> state;
    const BOOL ownerEnabled = IsWindowEnabled(owner);
    try
    {
        state = std::make_unique<WindowsBenchmarkSummaryReviewState>();
        state->frozen = frozen;
        WNDCLASSEXW cls{}; cls.cbSize = sizeof(cls); cls.lpfnWndProc = WindowsSummaryReviewWindowProc;
        cls.hInstance = GetModuleHandleW(nullptr); cls.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1); cls.lpszClassName = kSummaryReviewWindowClass;
        if (!RegisterClassExW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return;
        RECT ownerRect{}; GetWindowRect(owner, &ownerRect);
        MONITORINFO monitor{sizeof(MONITORINFO)};
        if (!GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitor)) return;
        const int width = std::min(SummaryReviewScale(owner, 800), static_cast<int>(monitor.rcWork.right - monitor.rcWork.left));
        const int height = std::min(SummaryReviewScale(owner, 800), static_cast<int>(monitor.rcWork.bottom - monitor.rcWork.top));
        const int x = std::clamp(static_cast<int>(ownerRect.left + (ownerRect.right - ownerRect.left - width) / 2),
            static_cast<int>(monitor.rcWork.left), static_cast<int>(monitor.rcWork.right) - width);
        const int y = std::clamp(static_cast<int>(ownerRect.top + (ownerRect.bottom - ownerRect.top - height) / 2),
            static_cast<int>(monitor.rcWork.top), static_cast<int>(monitor.rcWork.bottom) - height);
        window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kSummaryReviewWindowClass, L"Review statistics - local JSON",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN | WS_VSCROLL,
            x, y, width, height, owner, nullptr, GetModuleHandleW(nullptr), state.get());
        if (!window) return;
        EnableWindow(owner, FALSE); ShowWindow(window, SW_SHOW); UpdateWindow(window);
        SetFocus(SummaryReviewControl(window, kSummaryConsent));
        MSG message{};
        while (IsWindow(window) && IsWindow(owner))
        {
            const BOOL result = GetMessageW(&message, nullptr, 0, 0);
            if (result <= 0) { if (result == 0) PostQuitMessage(static_cast<int>(message.wParam)); break; }
            if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE)
            {
                HWND cooling = SummaryReviewControl(window, kSummaryCooling);
                if (SendMessageW(cooling, CB_GETDROPPEDSTATE, 0, 0)) SendMessageW(cooling, CB_SHOWDROPDOWN, FALSE, 0);
                else DestroyWindow(window);
                continue;
            }
            if (!IsDialogMessageW(window, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
        }
        if (IsWindow(window)) DestroyWindow(window);
        window = nullptr;
    }
    catch (...)
    {
        if (IsWindow(window)) DestroyWindow(window);
        if (IsWindow(owner)) MessageBoxW(owner, L"The optional local statistics form is unavailable. The benchmark result is unchanged.",
            L"Horde Lantern RT", MB_OK | MB_ICONINFORMATION);
    }
    if (IsWindow(owner) && ownerEnabled) { EnableWindow(owner, TRUE); SetForegroundWindow(owner); }
}
