#pragma once
#include "graphics/GraphicsSettings.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace horde::platform::windows
{
// The confirmed tuple and pending marker move together. A failed write keeps
// the previous file; copying the INI preserves unrelated settings sections.
inline bool SaveGraphicsPersistenceRecord(const std::filesystem::path& settingsPath,
    const horde::graphics::GraphicsPersistenceRecord& record)
{
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
    }
    success = write("confirmedScale", record.confirmed.renderScalePercent) && success;
    success = write("confirmedWater", static_cast<int>(record.confirmed.waterQuality)) && success;
    success = write("confirmedFire", static_cast<int>(record.confirmed.fireDetail)) && success;
    success = write("confirmedCap", record.confirmed.previewFrameCap) && success;
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
