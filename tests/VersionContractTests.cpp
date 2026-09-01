#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

namespace
{
namespace fs = std::filesystem;

constexpr const char* kSourceDirectory = HORDE_RT_SOURCE_DIR;
constexpr const char* kExpectedVersion = HORDE_RT_EXPECTED_VERSION;
constexpr int kExpectedAndroidVersionCode = HORDE_RT_EXPECTED_ANDROID_VERSION_CODE;

bool ReadFile(const fs::path& path, std::string& content)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::cerr << "FAIL: could not read " << path.string() << '\n';
        return false;
    }
    content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

bool Require(const bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

bool Contains(const std::string& content, const std::string& expected)
{
    return content.find(expected) != std::string::npos;
}

bool CheckSourceContains(const fs::path& path, const std::string& expected, const std::string& description)
{
    std::string content;
    return ReadFile(path, content) && Require(Contains(content, expected), description + ": " + path.string());
}
}

int main()
{
    bool passed = true;
    const fs::path sourceRoot(kSourceDirectory);
    std::string versionRaw;
    passed &= ReadFile(sourceRoot / "VERSION", versionRaw);
    const std::regex strictVersion("^[0-9]+\\.[0-9]+\\.[0-9]+\\r?\\n?$");
    passed &= Require(std::regex_match(versionRaw, strictVersion), "VERSION must be a single strict SemVer line");
    passed &= Require(std::regex_match(std::string("1.6.1\r\n"), strictVersion),
        "strict version reader must allow a Windows CRLF checkout");
    passed &= Require(!std::regex_match(std::string("1.6.1\n\n"), strictVersion),
        "strict version reader must reject a second blank line");
    const std::string version = versionRaw.substr(0, versionRaw.find_first_of("\r\n"));
    passed &= Require(version == kExpectedVersion, "CMake-resolved semantic version must equal VERSION");

    std::string versionMap;
    passed &= ReadFile(sourceRoot / "version-code-map.json", versionMap);
    const std::regex activeMap("\\\"" + version + "\\\"\\s*:\\s*([1-9][0-9]*)\\s*(?=[,}])");
    std::smatch activeMapMatch;
    passed &= Require(std::regex_search(versionMap, activeMapMatch, activeMap), "Android version-code map must contain the active semantic version");
    if (activeMapMatch.size() == 2)
    {
        passed &= Require(std::stoi(activeMapMatch[1].str()) == kExpectedAndroidVersionCode,
            "CMake-resolved Android version code must equal the map entry");
    }

    const fs::path cmakeLists = sourceRoot / "CMakeLists.txt";
    passed &= CheckSourceContains(cmakeLists, "include(cmake/HordeRtVersion.cmake)",
        "CMake must read the shared root version contract before project()");
    passed &= CheckSourceContains(cmakeLists, "VERSION ${HORDE_RT_PACKAGE_VERSION}",
        "Root project version must use the shared contract");
    passed &= CheckSourceContains(cmakeLists, "HordeLanternRT.rc.in",
        "Windows resource must be configured from its template");
    passed &= CheckSourceContains(cmakeLists, "HordeLanternRT.manifest.in",
        "Windows manifest must be configured from its template");

    const fs::path gradle = sourceRoot / "android/app/build.gradle";
    passed &= CheckSourceContains(gradle, "rootProject.file('../VERSION')",
        "Gradle must read the root semantic version");
    passed &= CheckSourceContains(gradle, "rootProject.file('../version-code-map.json')",
        "Gradle must read the shared Android version-code map");
    passed &= CheckSourceContains(gradle, "versionCode hordeVersionCode.intValue()",
        "Gradle versionCode must use the mapped active value");
    passed &= CheckSourceContains(gradle, "versionName hordeVersionName",
        "Gradle versionName must use the root semantic version");
    passed &= CheckSourceContains(gradle, "resValue 'string', 'alpha_version', \"SHOWCASE · ALPHA ${hordeVersionName}\"",
        "Android display resource must use the active semantic version");

    const fs::path nativeCmake = sourceRoot / "android/app/src/main/cpp/CMakeLists.txt";
    passed &= CheckSourceContains(nativeCmake, "cmake/HordeRtVersion.cmake",
        "Android nested CMake must include the root version contract");

    const fs::path rcTemplate = sourceRoot / "src/platform/windows/HordeLanternRT.rc.in";
    passed &= CheckSourceContains(rcTemplate, "FILEVERSION @HORDE_RT_WINDOWS_FILE_VERSION@",
        "Windows RC template must use parsed four-part numeric version");
    passed &= CheckSourceContains(rcTemplate, "ProductVersion\", \"@HORDE_RT_DISPLAY_VERSION@\\0\"",
        "Windows RC product version must use the semantic version");
    const fs::path manifestTemplate = sourceRoot / "src/platform/windows/HordeLanternRT.manifest.in";
    passed &= CheckSourceContains(manifestTemplate, "version=\"@HORDE_RT_WINDOWS_ASSEMBLY_VERSION@\"",
        "Windows manifest template must use parsed four-part version");
    passed &= CheckSourceContains(manifestTemplate, "PerMonitorV2,PerMonitor",
        "Windows manifest must retain PerMonitorV2");
    passed &= CheckSourceContains(manifestTemplate, "longPathAware",
        "Windows manifest must retain long-path awareness");
    passed &= CheckSourceContains(manifestTemplate, "Microsoft.Windows.Common-Controls",
        "Windows manifest must retain the common-controls dependency");

    const std::string releasePackageMarker = "Package version: `" + version + "`";
    const std::string releaseCodeMarker = "Android version code: `" + std::to_string(kExpectedAndroidVersionCode) + "`";
    int matchingReleaseNotes = 0;
    for (const auto& entry : fs::directory_iterator(sourceRoot / "docs"))
    {
        if (!entry.is_regular_file() || entry.path().filename().string().find("RELEASE_NOTES") == std::string::npos)
        {
            continue;
        }
        std::string notes;
        if (ReadFile(entry.path(), notes) && Contains(notes, releasePackageMarker) && Contains(notes, releaseCodeMarker))
        {
            ++matchingReleaseNotes;
        }
    }
    passed &= Require(matchingReleaseNotes == 1, "exactly one active release-note file must match the source contract");

    for (const auto& relativeReadme : {fs::path("README.md"), fs::path("android/README.md"), fs::path("release/windows/README.txt")})
    {
        std::string readme;
        passed &= ReadFile(sourceRoot / relativeReadme, readme);
        passed &= Require(Contains(readme, version), "current README surface must name the active version: " + relativeReadme.string());
        passed &= Require(Contains(readme, "1.6.0"), "current README surface must preserve the latest published release: " + relativeReadme.string());
    }

    const fs::path packagePolicy = sourceRoot / "tools/release-version-policy.ps1";
    passed &= CheckSourceContains(packagePolicy, "version-contract.ps1",
        "release policy must read the shared source identity");
    passed &= CheckSourceContains(packagePolicy, "Assert-HordeSourceIdentityMatches",
        "release policy must reject version/code drift");
    for (const auto& script : {"tools/package-alpha.ps1", "tools/package-signed-alpha.ps1", "tools/push-alpha-to-itch.ps1"})
    {
        passed &= CheckSourceContains(sourceRoot / script, "Assert-HordeReleaseVersionIsMutable",
            "candidate, signing, and upload paths must use the shared release policy");
        passed &= CheckSourceContains(sourceRoot / script, "Horde-Lantern-RT-Alpha-$safeVersion",
            "candidate names must derive from the explicit active-version argument");
    }

    if (!passed)
    {
        return 1;
    }
    std::cout << "Version contract tests passed for " << version << " / Android code " << kExpectedAndroidVersionCode << ".\n";
    return 0;
}
