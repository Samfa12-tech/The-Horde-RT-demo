#include "platform/windows/WindowsGraphicsPersistence.h"
#include <iostream>

int main()
{
    using namespace horde::graphics;
    const auto directory = std::filesystem::temp_directory_path() /
        ("horde-graphics-persistence-" + std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(directory);
    const auto path = directory / "settings.ini";
    const auto filename = path.string();
    { std::ofstream file(path); file << "[audio]\nvolume=43\n[controls]\nsensitivity=135\n"; }
    bool passed = true;
    const auto check = [&passed](bool condition, const char* message) {
        if (!condition) { std::cerr << message << '\n'; passed = false; }
    };
    GraphicsPersistenceRecord record;
    record.confirmed = BaselineGraphicsSettings(GraphicsPlatform::Windows);
    record.pending = GraphicsSettings{63, WaterQuality::Off, FireDetail::Mobile, 30};
    check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "pending tuple saved atomically");
    const auto read = [&filename](const char* section, const char* key) {
        return GetPrivateProfileIntA(section, key, -1, filename.c_str());
    };
    check(read("graphics", "pending") == 1 && read("graphics", "pendingScale") == 63 &&
          read("graphics", "confirmedScale") == 100, "interrupted candidate retains complete last-confirmed tuple");
    check(read("audio", "volume") == 43 && read("controls", "sensitivity") == 135,
          "graphics transaction preserves unrelated INI sections");
    record.confirmed = *record.pending; record.pending.reset();
    check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "confirmation tuple saved atomically");
    check(read("graphics", "pending") == 0 && read("graphics", "confirmedScale") == 63 &&
          read("graphics", "confirmedWater") == 0 && read("graphics", "confirmedFire") == 0,
          "confirmation clears marker and publishes complete requested tuple");
    check(!horde::platform::windows::SaveGraphicsPersistenceRecord(directory / "missing" / "settings.ini", record),
          "write failure is explicit");
    check(read("graphics", "confirmedScale") == 63, "failure preserves last usable settings");
    const HANDLE locked = CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    check(locked != INVALID_HANDLE_VALUE, "fixture locks original against replacement");
    if (locked != INVALID_HANDLE_VALUE)
    {
        record.confirmed.renderScalePercent = 55;
        check(!horde::platform::windows::SaveGraphicsPersistenceRecord(path, record),
              "failed atomic publication is explicit");
        CloseHandle(locked);
        check(read("graphics", "confirmedScale") == 63 && read("audio", "volume") == 43,
              "failed publication preserves the entire prior record and unrelated preferences");
    }
    std::filesystem::remove(path);
    std::filesystem::remove(directory);
    return passed ? 0 : 1;
}
