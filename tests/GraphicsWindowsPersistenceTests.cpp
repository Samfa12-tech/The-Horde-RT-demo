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
    record.pending = GraphicsSettings{63, WaterQuality::Off, FireDetail::Low, 30, false, ShadowQuality::Lower, false, DustQuality::Standard};
    check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "pending tuple saved atomically");
    const auto read = [&filename](const char* section, const char* key) {
        return GetPrivateProfileIntA(section, key, -1, filename.c_str());
    };
    check(read("graphics", "pending") == 1 && read("graphics", "pendingScale") == 63 &&
          read("graphics", "confirmedScale") == 100 && read("graphics", "pendingGlass") == 0 &&
          read("graphics", "confirmedGlass") == 1 && read("graphics", "schema") == 5 &&
          read("graphics", "pendingFire") == 2 && read("graphics", "pendingShadow") == 0 &&
          read("graphics", "confirmedShadow") == 1 && read("graphics", "confirmedMist") == 1 && read("graphics", "pendingMist") == 0 &&
          read("graphics", "confirmedDust") == 0 && read("graphics", "pendingDust") == 2,
          "interrupted settings candidate retains complete last-confirmed tuple and Standard dust intent");
    const auto loadedPending = horde::platform::windows::LoadGraphicsPersistenceRecord(path, record.confirmed);
    check(loadedPending && loadedPending->confirmed == record.confirmed && loadedPending->pending == record.pending,
          "ordinary owner loader reads exact confirmed and pending settings tuples");
    if (loadedPending)
    {
        const auto recovery = RecoverGraphicsSettings(*loadedPending, GraphicsPlatform::Windows);
        check(recovery.startup == record.confirmed && recovery.retainedRequested == record.pending &&
            HasGraphicsReason(recovery.reasons, GraphicsReason::InterruptedApply),
            "interrupted Low/Lower loads confirmed baseline while retaining unconfirmed intent");
    }
    check(read("audio", "volume") == 43 && read("controls", "sensitivity") == 135,
          "graphics transaction preserves unrelated INI sections");
    record.confirmed = *record.pending; record.pending.reset();
    check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "confirmation tuple saved atomically");
    check(read("graphics", "pending") == 0 && read("graphics", "confirmedScale") == 63 &&
          read("graphics", "confirmedWater") == 0 && read("graphics", "confirmedFire") == 2 &&
          read("graphics", "confirmedGlass") == 0 && read("graphics", "confirmedShadow") == 0 && read("graphics", "confirmedMist") == 0 &&
          read("graphics", "confirmedDust") == 2,
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
        record.confirmed.glassEnabled = true;
        record.confirmed.shadowQuality = ShadowQuality::Higher;
        check(!horde::platform::windows::SaveGraphicsPersistenceRecord(path, record),
              "failed atomic publication is explicit");
        CloseHandle(locked);
        check(read("graphics", "confirmedScale") == 63 && read("graphics", "confirmedGlass") == 0 &&
              read("graphics", "confirmedFire") == 2 && read("graphics", "confirmedShadow") == 0 &&
              read("audio", "volume") == 43,
              "failed publication preserves the entire prior record and unrelated preferences");
    }
    record.confirmed = GraphicsSettings{63, WaterQuality::Off, FireDetail::Low, 30, false, ShadowQuality::Lower, false, DustQuality::Standard};
    auto invalid = record;
    invalid.confirmed.shadowQuality = static_cast<ShadowQuality>(3);
    check(!horde::platform::windows::SaveGraphicsPersistenceRecord(path, invalid) &&
        read("graphics", "confirmedShadow") == 0,
        "unknown shadow cannot replace a usable seven-field record");
    const auto write = [&](const char* key, const char* value) {
        check(WritePrivateProfileStringA("graphics", key, value, filename.c_str()) != FALSE, "fixture INI key written");
    };
    const auto baseline = BaselineGraphicsSettings(GraphicsPlatform::Windows);
    const auto recover = [&]() {
        const auto loaded = horde::platform::windows::LoadGraphicsPersistenceRecord(path, baseline);
        check(loaded.has_value(), "stored graphics schema is present");
        return loaded ? RecoverGraphicsSettings(*loaded, GraphicsPlatform::Windows) : GraphicsRecovery{};
    };
    // Keep the legacy Fire value valid in both tuples while exercising stale
    // confirmed and pending Dust keys under schemas that predate Dust.
    record.confirmed.fireDetail = FireDetail::High;
    record.pending = record.confirmed;
    for (const char* schema : {"1", "2"})
    {
        check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "migration source seeded");
        write("schema", schema); write("confirmedFire", "1"); write("confirmedShadow", "256"); write("confirmedDust", "2"); write("pendingDust", "1");
        const auto migrated = recover();
        check(migrated.startup.renderScalePercent == 63 && migrated.startup.waterQuality == WaterQuality::Off &&
            migrated.startup.fireDetail == FireDetail::High && migrated.startup.previewFrameCap == 30 &&
            migrated.startup.shadowQuality == ShadowQuality::Current &&
            migrated.startup.glassEnabled == (schema[0] == '1'),
            "schema1/2 preserve independent old fire values, migrate Current shadow and exact glass semantics");
        check(migrated.startup.dustQuality == DustQuality::Off && migrated.retainedRequested &&
            migrated.retainedRequested->dustQuality == DustQuality::Off,
            "schema1/2 ignore stale confirmed and pending dust keys");
        write("confirmedFire", "2");
        check(HasGraphicsReason(recover().reasons, GraphicsReason::InvalidStoredSettings),
            "old schema cannot reinterpret previously unknown Fire2 as new Low");
    }
    record.pending.reset();
    for (const auto quality : {ShadowQuality::Lower, ShadowQuality::Current, ShadowQuality::Higher})
    {
        record.confirmed.shadowQuality = quality;
        check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "all new shadow selections save");
        const auto current = recover();
        check(current.startup == record.confirmed && current.reasons == GraphicsReason::None,
            "schema4 Low retains all three exact shadow selections without remapping or unrelated changes");
    }
    for (const char* schema : {"1", "2", "3"})
    {
        check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "old Mist migration source seeded");
        write("schema", schema); write("confirmedFire", "1"); write("confirmedMist", "invalid-stale-key");
        write("confirmedDust", "2");
        const auto migrated = recover();
        check(migrated.startup.renderScalePercent == 63 && migrated.startup.mistEnabled &&
              migrated.startup.dustQuality == DustQuality::Off &&
              !HasGraphicsReason(migrated.reasons, GraphicsReason::InvalidStoredSettings),
              "pre4 schemas ignore stale Mist and dust keys and preserve saved quality with MistOn/DustOff");
    }
    check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "schema4 missing Mist source seeded");
    write("schema", "4");
    write("confirmedMist", nullptr);
    check(recover().startup.mistEnabled && !HasGraphicsReason(recover().reasons, GraphicsReason::InvalidStoredSettings),
          "missing schema4 Mist defaultsOn without replacing other saved tuple fields");
    check(recover().startup.dustQuality == DustQuality::Off,
          "schema4 ignores stale dust key and migrates to Off");
    for (const char* malformed : {"-1", "2", "256", "01", "On", "1junk"})
    {
        check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "Mist negative source seeded");
        write("schema", "4");
        write("confirmedMist", malformed);
        check(HasGraphicsReason(recover().reasons, GraphicsReason::InvalidStoredSettings),
              "schema4 Mist parser rejects every noncanonical boolean before native apply");
    }
    for (const char* malformed : {"", "-1", "3", "256", "01", "Current", "1junk", "999999999999999999999999999999999999"})
    {
        check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "malformed test source seeded");
        write("confirmedShadow", malformed);
        const auto rejected = recover();
        check(rejected.startup == baseline && HasGraphicsReason(rejected.reasons, GraphicsReason::InvalidStoredSettings),
            "missing/malformed/overflow shadow never coerces or wraps into an admitted tuple");
    }
    for (const char* malformed : {"-1", "3", "256", "01", "Low", "1junk"})
    {
        check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "dust enum negative source seeded");
        write("confirmedDust", malformed);
        check(HasGraphicsReason(recover().reasons, GraphicsReason::InvalidStoredSettings),
              "schema5 rejects malformed/noncanonical dust enum before native apply");
    }
    check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "schema5 missing dust source seeded");
    write("confirmedDust", nullptr);
    check(HasGraphicsReason(recover().reasons, GraphicsReason::InvalidStoredSettings),
          "schema5 requires confirmed dust field");
    check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "boolean negative source seeded");
    write("confirmedGlass", "01");
    check(HasGraphicsReason(recover().reasons, GraphicsReason::InvalidStoredSettings),
        "boolean parser still requires exact0/1 rather than numeric coercion");
    record.pending = record.confirmed;
    record.pending->shadowQuality = ShadowQuality::Lower;
    check(horde::platform::windows::SaveGraphicsPersistenceRecord(path, record), "pending negative source seeded");
    write("pendingDust", nullptr);
    check(HasGraphicsReason(recover().reasons, GraphicsReason::InvalidStoredSettings),
        "schema5 pending dust is independently required while its marker is active");
    write("pending", "0");
    check(recover().startup == record.confirmed && recover().reasons == GraphicsReason::None,
        "inactive missing pending fields do not invalidate confirmed settings");
    check(read("audio", "volume") == 43 && read("controls", "sensitivity") == 135,
        "migration/rejection and enum changes preserve unrelated settings");
    std::filesystem::remove(path);
    std::filesystem::remove(directory);
    return passed ? 0 : 1;
}
