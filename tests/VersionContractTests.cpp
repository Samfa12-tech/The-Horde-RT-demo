#include <charconv>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <string>
#include <vector>

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

int CountMatches(const std::string& content, const std::regex& pattern)
{
    return static_cast<int>(std::distance(std::sregex_iterator(content.begin(), content.end(), pattern),
                                          std::sregex_iterator()));
}

bool Contains(const std::string& content, const std::string& expected)
{
    return content.find(expected) != std::string::npos;
}

bool IsStrictVersionRaw(const std::string& value)
{
    static const std::regex strictVersion(
        "^((?:0|[1-9][0-9]*)\\.(?:0|[1-9][0-9]*)\\.(?:0|[1-9][0-9]*))(?:\\r?\\n)?$");
    return std::regex_match(value, strictVersion);
}

bool ParseActiveVersionCode(const std::string& map, const std::string& version, int& value)
{
    if (map.starts_with("\xEF\xBB\xBF"))
    {
        return false;
    }
    const std::regex activeAssignment("\\\"" + version + "\\\"\\s*:\\s*([^,}\\s]+)");
    std::vector<std::string> tokens;
    for (std::sregex_iterator it(map.begin(), map.end(), activeAssignment), end; it != end; ++it)
    {
        tokens.push_back((*it)[1].str());
    }
    if (tokens.size() != 1 || !std::regex_match(tokens.front(), std::regex("^[1-9][0-9]*$")))
    {
        return false;
    }
    long long parsed = 0;
    const auto [ptr, error] = std::from_chars(tokens.front().data(), tokens.front().data() + tokens.front().size(), parsed);
    if (error != std::errc{} || ptr != tokens.front().data() + tokens.front().size() ||
        parsed > std::numeric_limits<int>::max())
    {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool HasSingleExactAssignment(const std::string& source, const std::regex& assignment, const std::string& expectedValue)
{
    std::vector<std::string> values;
    for (std::sregex_iterator it(source.begin(), source.end(), assignment), end; it != end; ++it)
    {
        values.push_back((*it)[1].str());
    }
    return values.size() == 1 && values.front() == expectedValue;
}

bool HasSingleExactLineAssignment(const std::string& source, const std::string& name, const std::string& expectedValue)
{
    int matches = 0;
    std::string value;
    size_t lineStart = 0;
    while (lineStart < source.size())
    {
        const size_t lineEnd = source.find('\n', lineStart);
        std::string line = source.substr(lineStart, lineEnd == std::string::npos ? std::string::npos : lineEnd - lineStart);
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        const size_t first = line.find_first_not_of(" \t");
        if (first != std::string::npos && line.compare(first, name.size(), name) == 0 &&
            first + name.size() < line.size() && std::isspace(static_cast<unsigned char>(line[first + name.size()])))
        {
            ++matches;
            const size_t valueStart = line.find_first_not_of(" \t", first + name.size());
            value = valueStart == std::string::npos ? "" : line.substr(valueStart);
        }
        if (lineEnd == std::string::npos)
        {
            break;
        }
        lineStart = lineEnd + 1;
    }
    return matches == 1 && value == expectedValue;
}
}

int main()
{
    bool passed = true;
    const fs::path sourceRoot(kSourceDirectory);
    std::string versionRaw;
    passed &= ReadFile(sourceRoot / "VERSION", versionRaw);
    passed &= Require(IsStrictVersionRaw(versionRaw), "VERSION must be a single strict SemVer line");
    passed &= Require(IsStrictVersionRaw("1.6.1\r\n"), "strict version reader must allow a Windows CRLF checkout");
    for (const auto& invalidVersion : {std::string("01.6.1\n"), std::string("1.06.1\n"), std::string("1.6.01\n"),
                                       std::string("1.6.1\n\n"), std::string("\xEF\xBB\xBF" "1.6.1\n")})
    {
        passed &= Require(!IsStrictVersionRaw(invalidVersion), "strict version reader must reject malformed SemVer bytes");
    }
    const std::string version = versionRaw.substr(0, versionRaw.find_first_of("\r\n"));
    passed &= Require(version == kExpectedVersion, "CMake-resolved semantic version must equal VERSION");

    std::string versionMap;
    passed &= ReadFile(sourceRoot / "version-code-map.json", versionMap);
    int activeVersionCode = 0;
    passed &= Require(ParseActiveVersionCode(versionMap, version, activeVersionCode),
        "Android version-code map must contain exactly one active positive integer assignment");
    passed &= Require(activeVersionCode == kExpectedAndroidVersionCode,
        "CMake-resolved Android version code must equal the unique active map assignment");
    for (const auto& invalidMap : {
             std::string("{\"androidVersionCodes\":{\"1.6.1\":9.0}}"),
             std::string("{\"androidVersionCodes\":{\"1.6.1\":9e0}}"),
             std::string("{\"androidVersionCodes\":{\"1.6.1\":\"9\"}}"),
             std::string("{\"androidVersionCodes\":{\"1.6.1\":2147483648}}"),
             std::string("{\"androidVersionCodes\":{\"1.6.1\":9,\"1.6.1\":10}}"),
             std::string("\xEF\xBB\xBF" "{\"androidVersionCodes\":{\"1.6.1\":9}}")})
    {
        int ignored = 0;
        passed &= Require(!ParseActiveVersionCode(invalidMap, "1.6.1", ignored),
            "strict map reader must reject decimal, exponent, string, overflow, and duplicate active assignments");
    }

    std::string cmakeVersion;
    passed &= ReadFile(sourceRoot / "cmake" / "HordeRtVersion.cmake", cmakeVersion);
    passed &= Require(Contains(cmakeVersion, "CMAKE_CONFIGURE_DEPENDS") &&
                      Contains(cmakeVersion, "${_horde_rt_version_file}") &&
                      Contains(cmakeVersion, "${_horde_rt_version_code_map_file}"),
        "shared CMake reader must make both authorities configure dependencies");
    passed &= Require(Contains(cmakeVersion, "(0|[1-9][0-9]*)"),
        "CMake reader must reject leading-zero SemVer components");
    passed &= Require(Contains(cmakeVersion, "_horde_rt_active_map_assignment_count EQUAL 1"),
        "CMake reader must reject duplicate active map assignments");
    passed &= Require(Contains(cmakeVersion, "GREATER 2147483647"),
        "CMake reader must enforce Android-safe version-code range");

    std::string cmakeLists;
    std::string cmakeSources;
    std::string nestedAndroidCmake;
    passed &= ReadFile(sourceRoot / "CMakeLists.txt", cmakeLists);
    passed &= ReadFile(sourceRoot / "cmake" / "HordeRtSources.cmake", cmakeSources);
    passed &= ReadFile(sourceRoot / "android" / "app" / "src" / "main" / "cpp" / "CMakeLists.txt", nestedAndroidCmake);
    const std::regex projectVersion("project\\s*\\([\\s\\S]*?VERSION\\s+([^\\s\\)]+)");
    passed &= Require(HasSingleExactAssignment(cmakeLists, projectVersion, "${HORDE_RT_PACKAGE_VERSION}"),
        "root project must have exactly one effective dynamic VERSION assignment");
    const std::string duplicateProject = cmakeLists + "\nproject(Accidental VERSION 1.6.1 LANGUAGES NONE)\n";
    passed &= Require(!HasSingleExactAssignment(duplicateProject, projectVersion, "${HORDE_RT_PACKAGE_VERSION}"),
        "cross-platform contract check must fail when a second active CMake project version source appears");
    const std::regex cmakeIdentityOverride("set\\s*\\(\\s*HORDE_RT_(?:PACKAGE_VERSION|DISPLAY_VERSION|BUILD_ID)\\s+");
    const std::string cmakeConsumers = cmakeLists + cmakeSources + nestedAndroidCmake;
    passed &= Require(CountMatches(cmakeConsumers, cmakeIdentityOverride) == 0,
        "root, source-list, and nested Android CMake must not override shared package/display/build identity values");
    passed &= Require(CountMatches(cmakeVersion, std::regex("list\\s*\\(\\s*GET\\s+_horde_rt_version_lines\\s+0\\s+HORDE_RT_PACKAGE_VERSION\\s*\\)")) == 1 &&
                      CountMatches(cmakeVersion, std::regex("set\\s*\\(\\s*HORDE_RT_DISPLAY_VERSION\\s+")) == 1 &&
                      CountMatches(cmakeVersion, std::regex("set\\s*\\(\\s*HORDE_RT_BUILD_ID\\s+")) == 1,
        "shared CMake reader must have one package extraction and one display/build identity assignment");
    passed &= Require(CountMatches(cmakeConsumers + "\nset(HORDE_RT_PACKAGE_VERSION \"1.6.1\")\n", cmakeIdentityOverride) == 1,
        "cross-platform contract check must detect an effective CMake identity override");

    std::string gradle;
    passed &= ReadFile(sourceRoot / "android" / "app" / "build.gradle", gradle);
    passed &= Require(HasSingleExactLineAssignment(gradle, "versionName", "hordeVersionName"),
        "Gradle must have exactly one effective dynamic versionName assignment");
    passed &= Require(HasSingleExactLineAssignment(gradle, "versionCode", "hordeVersionCode.intValue()"),
        "Gradle must have exactly one effective dynamic versionCode assignment");
    passed &= Require(!HasSingleExactLineAssignment(gradle + "\n        versionName '1.6.1'\n", "versionName", "hordeVersionName"),
        "cross-platform contract check must fail when a second active Gradle version source appears");
    const std::regex gradleDisplayResource("resValue\\s+'string',\\s*'alpha_version',\\s*\\\"SHOWCASE · ALPHA \\$\\{hordeVersionName\\}\\\"");
    passed &= Require(CountMatches(gradle, gradleDisplayResource) == 1,
        "Gradle must have exactly one dynamic alpha_version resource assignment");
    passed &= Require(CountMatches(gradle + "\nresValue 'string', 'alpha_version', 'SHOWCASE · ALPHA 1.6.1'\n", gradleDisplayResource) == 1 &&
                      CountMatches(gradle + "\nresValue 'string', 'alpha_version', 'SHOWCASE · ALPHA 1.6.1'\n",
                          std::regex("resValue\\s+'string',\\s*'alpha_version'")) == 2,
        "cross-platform contract check must detect a second active alpha_version source");
    passed &= Require(Contains(gradle, "readHordeUtf8Authority") && Contains(gradle, "hordeActiveAssignmentCount != 1") &&
                      Contains(gradle, "hordeVersionCode instanceof Integer"),
        "Gradle must enforce raw UTF-8, unique active assignment, and integer code type");

    std::string powerShell;
    passed &= ReadFile(sourceRoot / "tools" / "version-contract.ps1", powerShell);
    passed &= Require(Contains(powerShell, "ReadAllBytes") && Contains(powerShell, "byte-order mark") &&
                      Contains(powerShell, "activeAssignmentCount -ne 1") && Contains(powerShell, "-isnot [long]"),
        "PowerShell must enforce raw UTF-8, unique active assignment, and integral code type");

    passed &= Require(CountMatches(nestedAndroidCmake,
                          std::regex("include\\(\\\"?\\$\\{HORDE_RT_REPO_ROOT\\}/cmake/HordeRtVersion\\.cmake\\\"?\\)")) == 1,
        "Android nested CMake must include the shared version reader exactly once");
    passed &= Require(Contains(nestedAndroidCmake, "HORDE_RT_PACKAGE_VERSION=\"${HORDE_RT_PACKAGE_VERSION}\""),
        "Android native compile definitions must derive the package version from the shared reader");
    passed &= Require(CountMatches(cmakeLists, std::regex("configure_file\\(src/platform/windows/HordeLanternRT\\.rc\\.in")) == 1 &&
                      CountMatches(cmakeLists, std::regex("configure_file\\(src/platform/windows/HordeLanternRT\\.manifest\\.in")) == 1,
        "root CMake must configure exactly one Windows RC and manifest from dynamic templates");

    for (const auto& relativeReadme : {fs::path("README.md"), fs::path("android/README.md"), fs::path("release/windows/README.txt")})
    {
        std::string readme;
        passed &= ReadFile(sourceRoot / relativeReadme, readme);
        passed &= Require(Contains(readme, version), "current README surface must name the active version: " + relativeReadme.string());
        passed &= Require(Contains(readme, "1.6.0"), "current README surface must preserve the latest published release: " + relativeReadme.string());
    }

    std::string releasePolicy;
    passed &= ReadFile(sourceRoot / "tools" / "release-version-policy.ps1", releasePolicy);
    passed &= Require(Contains(releasePolicy, "version-contract.ps1") && Contains(releasePolicy, "Assert-HordeSourceIdentityMatches"),
        "release policy must read and enforce the shared source identity");
    for (const auto& script : {fs::path("tools/package-alpha.ps1"), fs::path("tools/package-signed-alpha.ps1"),
                               fs::path("tools/push-alpha-to-itch.ps1")})
    {
        std::string content;
        passed &= ReadFile(sourceRoot / script, content);
        passed &= Require(Contains(content, "Assert-HordeReleaseVersionIsMutable"),
            "candidate, signing, and upload paths must use the shared release policy: " + script.string());
        passed &= Require(Contains(content, "Horde-Lantern-RT-Alpha-$safeVersion"),
            "candidate naming must derive from the explicit active-version argument: " + script.string());
    }

    const fs::path rcTemplate = sourceRoot / "src" / "platform" / "windows" / "HordeLanternRT.rc.in";
    const fs::path manifestTemplate = sourceRoot / "src" / "platform" / "windows" / "HordeLanternRT.manifest.in";
    std::string rc;
    std::string manifest;
    passed &= ReadFile(rcTemplate, rc) && ReadFile(manifestTemplate, manifest);
    passed &= Require(CountMatches(rc, std::regex("FILEVERSION\\s+@HORDE_RT_WINDOWS_FILE_VERSION@")) == 1 &&
                      CountMatches(rc, std::regex("PRODUCTVERSION\\s+@HORDE_RT_WINDOWS_FILE_VERSION@")) == 1 &&
                      CountMatches(rc, std::regex("FileVersion\\\",\\s*\\\"@HORDE_RT_DISPLAY_VERSION@\\\\0")) == 1 &&
                      CountMatches(rc, std::regex("ProductVersion\\\",\\s*\\\"@HORDE_RT_DISPLAY_VERSION@\\\\0")) == 1,
        "Windows RC must have one dynamic file/product numeric and string version assignment");
    passed &= Require(CountMatches(manifest, std::regex("version=\\\"@HORDE_RT_WINDOWS_ASSEMBLY_VERSION@\\\"")) == 1 &&
                      Contains(manifest, "PerMonitorV2,PerMonitor") && Contains(manifest, "longPathAware") &&
                      Contains(manifest, "Microsoft.Windows.Common-Controls"),
        "Windows manifest must have one dynamic identity and retain compatibility declarations");

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

    if (!passed)
    {
        return 1;
    }
    std::cout << "Version contract tests passed for " << version << " / Android code " << kExpectedAndroidVersionCode << ".\n";
    return 0;
}
