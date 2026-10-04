#pragma once
#include "graphics/GraphicsSettings.h"
#include <windows.h>
#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>

namespace horde::platform::windows
{
// Read the same INI tuple used by the ordinary owner. Legacy schemas cannot
// opt into new enum values through stale keys; malformed new keys are explicit.
inline std::optional<horde::graphics::GraphicsPersistenceRecord> LoadGraphicsPersistenceRecord(
    const std::filesystem::path& settingsPath, const horde::graphics::GraphicsSettings& legacy)
{
    using namespace horde::graphics;
    const auto filename = settingsPath.string();
    bool valid = true;
    const auto read = [&](const char* key, const int fallback, const bool required = false) {
        std::array<char, 32u> text{};
        const DWORD length = GetPrivateProfileStringA("graphics", key, "", text.data(),
            static_cast<DWORD>(text.size()), filename.c_str());
        if (length == 0u) { if (required) valid = false; return fallback; }
        int value = 0;
        const auto parsed = std::from_chars(text.data(), text.data() + length, value);
        if (length >= text.size() - 1u || parsed.ec != std::errc{} || parsed.ptr != text.data() + length ||
            std::to_string(value) != std::string(text.data(), length))
        { valid = false; return fallback; }
        return value;
    };
    std::array<char, 32u> schemaText{};
    if (GetPrivateProfileStringA("graphics", "schema", "", schemaText.data(),
        static_cast<DWORD>(schemaText.size()), filename.c_str()) == 0u) return std::nullopt;
    GraphicsPersistenceRecord record;
    const int schema = read("schema", 0, true);
    record.schema = schema > 0 ? static_cast<std::uint32_t>(schema) : 0u;
    const bool historical = schema == 1 || schema == 2;
    const auto tuple = [&](const bool pending) {
        GraphicsSettings settings;
        settings.renderScalePercent = read(pending ? "pendingScale" : "confirmedScale", legacy.renderScalePercent);
        const int water = read(pending ? "pendingWater" : "confirmedWater", static_cast<int>(legacy.waterQuality));
        const int fire = read(pending ? "pendingFire" : "confirmedFire", static_cast<int>(legacy.fireDetail));
        settings.previewFrameCap = read(pending ? "pendingCap" : "confirmedCap", 30);
        const int glass = schema == 1 ? 1 : read(pending ? "pendingGlass" : "confirmedGlass", 1, true);
        const int shadow = historical ? 1 : read(pending ? "pendingShadow" : "confirmedShadow", 1, true);
        if (water < 0 || water > 2 || fire < 0 || fire > (historical ? 1 : 2) ||
            glass < 0 || glass > 1 || shadow < 0 || shadow > 2) valid = false;
        settings.waterQuality = static_cast<WaterQuality>(water >= 0 && water <= 2 ? water : 0);
        settings.fireDetail = static_cast<FireDetail>(fire >= 0 && fire <= 2 ? fire : 0);
        settings.glassEnabled = glass == 1;
        settings.shadowQuality = static_cast<ShadowQuality>(shadow >= 0 && shadow <= 2 ? shadow : 1);
        return settings;
    };
    record.confirmed = tuple(false);
    const int pending = read("pending", 0);
    if (pending < 0 || pending > 1) valid = false;
    if (pending != 0) record.pending = tuple(true);
    if (!valid) record.schema = 0u;
    return record;
}

// The confirmed tuple and pending marker move together. A failed write keeps
// the previous file; copying the INI preserves unrelated settings sections.
inline bool SaveGraphicsPersistenceRecord(const std::filesystem::path& settingsPath,
    const horde::graphics::GraphicsPersistenceRecord& record)
{
    if (record.schema != horde::graphics::kGraphicsSettingsSchema ||
        !horde::graphics::ValidGraphicsSettings(record.confirmed) ||
        (record.pending && !horde::graphics::ValidGraphicsSettings(*record.pending))) return false;
    const auto original = settingsPath.string();
    const auto temporary = original + ".graphics-" + std::to_string(GetCurrentProcessId()) + ".tmp";
    if (GetFileAttributesA(original.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        if (!CopyFileA(original.c_str(), temporary.c_str(), FALSE)) return false;
    }
    else
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream.good()) return false;
    }
    const auto write = [&temporary](const char* key, const int value) {
        const auto text = std::to_string(value);
        return WritePrivateProfileStringA("graphics", key, text.c_str(), temporary.c_str()) != FALSE;
    };
    bool success = write("schema", static_cast<int>(record.schema));
    success = write("pending", record.pending ? 1 : 0) && success;
    if (record.pending)
    {
        success = write("pendingScale", record.pending->renderScalePercent) && success;
        success = write("pendingWater", static_cast<int>(record.pending->waterQuality)) && success;
        success = write("pendingFire", static_cast<int>(record.pending->fireDetail)) && success;
        success = write("pendingCap", record.pending->previewFrameCap) && success;
        success = write("pendingGlass", record.pending->glassEnabled ? 1 : 0) && success;
        success = write("pendingShadow", static_cast<int>(record.pending->shadowQuality)) && success;
    }
    success = write("confirmedScale", record.confirmed.renderScalePercent) && success;
    success = write("confirmedWater", static_cast<int>(record.confirmed.waterQuality)) && success;
    success = write("confirmedFire", static_cast<int>(record.confirmed.fireDetail)) && success;
    success = write("confirmedCap", record.confirmed.previewFrameCap) && success;
    success = write("confirmedGlass", record.confirmed.glassEnabled ? 1 : 0) && success;
    success = write("confirmedShadow", static_cast<int>(record.confirmed.shadowQuality)) && success;
    // The special cache-flush call has no ordinary key-write success result.
    // Check the actual file flush separately before publishing the new tuple.
    (void)WritePrivateProfileStringA(nullptr, nullptr, nullptr, temporary.c_str());
    if (success)
    {
        const HANDLE file = CreateFileA(temporary.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        success = file != INVALID_HANDLE_VALUE;
        if (success) { success = FlushFileBuffers(file) != FALSE; CloseHandle(file); }
    }
    if (success)
        success = MoveFileExA(temporary.c_str(), original.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    if (!success) DeleteFileA(temporary.c_str());
    return success;
}
} // namespace horde::platform::windows
