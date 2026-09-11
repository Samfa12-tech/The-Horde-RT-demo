#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
namespace fs = std::filesystem;

constexpr const char* kSourceDirectory = HORDE_RT_SOURCE_DIR;
const char* kCmakeCommand = nullptr;

bool WriteFile(const fs::path& path, const std::string& content)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        std::cerr << "FAIL: could not write " << path.string() << '\n';
        return false;
    }
    file << content;
    return static_cast<bool>(file);
}

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

bool RunCmake(const std::string& arguments)
{
#ifdef _WIN32
    const std::string command = "cmd.exe /d /s /c \"\"" + std::string(kCmakeCommand) + "\" " + arguments + "\"";
#else
    const std::string command = "\"" + std::string(kCmakeCommand) + "\" " + arguments;
#endif
    if (std::system(command.c_str()) != 0)
    {
        std::cerr << "FAIL: command failed: " << command << '\n';
        return false;
    }
    return true;
}

bool RunCmakeExpectFailure(const std::string& arguments)
{
#ifdef _WIN32
    const std::string command = "cmd.exe /d /s /c \"\"" + std::string(kCmakeCommand) + "\" " + arguments + "\"";
#else
    const std::string command = "\"" + std::string(kCmakeCommand) + "\" " + arguments;
#endif
    if (std::system(command.c_str()) == 0)
    {
        std::cerr << "FAIL: command unexpectedly succeeded: " << command << '\n';
        return false;
    }
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

void TrimLineEnding(std::string& value)
{
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n'))
    {
        value.pop_back();
    }
}
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "FAIL: expected the configured CMake command as the only argument.\n";
        return 1;
    }
    kCmakeCommand = argv[1];
    const fs::path sourceRoot(kSourceDirectory);
    const auto uniqueSuffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    fs::path fixtureRoot;
    std::error_code error;
    bool reservedFixtureRoot = false;
    for (int attempt = 0; attempt < 32; ++attempt)
    {
        fixtureRoot = fs::temp_directory_path() /
            ("horde_rt_version_contract_incremental_fixture_" + uniqueSuffix + "_" + std::to_string(attempt));
        if (fs::create_directory(fixtureRoot, error))
        {
            reservedFixtureRoot = true;
            break;
        }
        if (error)
        {
            std::cerr << "FAIL: could not create fixture directory: " << error.message() << '\n';
            return 1;
        }
    }
    if (!reservedFixtureRoot)
    {
        std::cerr << "FAIL: could not reserve a unique fixture directory.\n";
        return 1;
    }
    const fs::path buildRoot = fixtureRoot / "build";

    bool passed = true;
    passed &= WriteFile(fixtureRoot / "VERSION", "1.6.1\n");
    passed &= WriteFile(fixtureRoot / "version-code-map.json",
        "{\n  \"androidVersionCodes\": { \"1.6.1\": 9, \"1.6.2\": 11 }\n}\n");
    fs::copy_file(sourceRoot / "cmake" / "HordeRtVersion.cmake", fixtureRoot / "HordeRtVersion.cmake",
        fs::copy_options::overwrite_existing, error);
    passed &= Require(!error, "could not copy HordeRtVersion.cmake into the fixture");
    passed &= WriteFile(fixtureRoot / "identity.in",
        "@PROJECT_VERSION@/@HORDE_RT_PACKAGE_VERSION@/@HORDE_RT_ANDROID_VERSION_CODE@\n");
    passed &= WriteFile(fixtureRoot / "CMakeLists.txt", R"cmake(cmake_minimum_required(VERSION 3.22)
set(HORDE_RT_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
include("${CMAKE_CURRENT_SOURCE_DIR}/HordeRtVersion.cmake")
project(HordeRtVersionFixture VERSION ${HORDE_RT_PACKAGE_VERSION} LANGUAGES NONE)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/identity.in" "${CMAKE_CURRENT_BINARY_DIR}/identity.txt" @ONLY)
add_custom_target(horde_rt_fixture_identity ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/identity.txt")
)cmake");

    const auto quote = [](const fs::path& path) { return "\"" + path.string() + "\""; };
    if (passed)
    {
        passed &= RunCmake("-S " + quote(fixtureRoot) + " -B " + quote(buildRoot));
        passed &= RunCmake("--build " + quote(buildRoot));
    }

    std::string identity;
    if (passed)
    {
        passed &= ReadFile(buildRoot / "identity.txt", identity);
        TrimLineEnding(identity);
        passed &= Require(identity == "1.6.1/1.6.1/9",
            "initial fixture identity must be generated from its authorities; got " + identity);
    }

    if (passed)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        passed &= WriteFile(fixtureRoot / "version-code-map.json",
            "{\n  \"androidVersionCodes\": { \"1.6.1\": 10, \"1.6.2\": 11 }\n}\n");
        passed &= RunCmake("--build " + quote(buildRoot));
        passed &= ReadFile(buildRoot / "identity.txt", identity);
        TrimLineEnding(identity);
        passed &= Require(identity == "1.6.1/1.6.1/10",
            "an ordinary existing-tree build must refresh generated Android code after only the map changes; got " + identity);
    }

    if (passed)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        passed &= WriteFile(fixtureRoot / "VERSION", "1.6.2\n");
        passed &= RunCmake("--build " + quote(buildRoot));
        passed &= ReadFile(buildRoot / "identity.txt", identity);
        TrimLineEnding(identity);
        passed &= Require(identity == "1.6.2/1.6.2/11",
            "an ordinary existing-tree build must refresh project and package identity after only VERSION changes; got " + identity);
    }

    if (passed)
    {
        const std::string validVersion = "1.6.1\n";
        const std::string validMap = "{\n  \"androidVersionCodes\": { \"1.6.1\": 9 }\n}\n";
        const std::vector<std::pair<std::string, std::string>> invalidAuthorities = {
            {std::string("\xEF\xBB\xBF") + validVersion, validMap},
            {std::string("\xFF\n", 2), validMap},
            {"01.6.1\n", validMap},
            {validVersion, std::string("\xEF\xBB\xBF") + validMap},
            {validVersion, "{\"androidVersionCodes\":{\"1.6.1\":9.0}}"},
            {validVersion, "{\"androidVersionCodes\":{\"1.6.1\":9e0}}"},
            {validVersion, "{\"androidVersionCodes\":{\"1.6.1\":\"9\"}}"},
            {validVersion, "{\"androidVersionCodes\":{\"1.6.1\":2147483648}}"},
            {validVersion, "{\"androidVersionCodes\":{\"1.6.1\":9,\"1.6.1\":10}}"},
        };
        for (size_t index = 0; index < invalidAuthorities.size(); ++index)
        {
            passed &= WriteFile(fixtureRoot / "VERSION", invalidAuthorities[index].first);
            passed &= WriteFile(fixtureRoot / "version-code-map.json", invalidAuthorities[index].second);
            passed &= RunCmakeExpectFailure("-S " + quote(fixtureRoot) + " -B " + quote(fixtureRoot / ("invalid-" + std::to_string(index))));
        }
    }

    fs::remove_all(fixtureRoot, error);
    return passed ? 0 : 1;
}
