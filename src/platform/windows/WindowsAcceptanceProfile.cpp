#include "platform/windows/WindowsAcceptanceProfile.h"
#include <algorithm>
#include <array>

namespace horde::platform::windows {
namespace {
namespace fs = std::filesystem;
constexpr std::string_view marker = "horde-windows-acceptance-profile-v1\n";
bool PlainDirectory(const fs::path& path) {
    const auto attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) &&
        !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}
bool PlainTree(const fs::path& root) {
    std::error_code ec;
    unsigned count = 0;
    fs::recursive_directory_iterator it(root, fs::directory_options::none, ec), end;
    for (; !ec && it != end; it.increment(ec)) {
        if (++count > 256) return false;
        const auto attributes = GetFileAttributesW(it->path().c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        if (attributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (!it->is_regular_file(ec) || ec || fs::hard_link_count(it->path(), ec) != 1 || ec) return false;
    }
    return !ec;
}
}
bool WindowsAcceptanceProfile::Configure(std::span<const std::wstring_view> args, bool enabled,
    const fs::path& reports, const fs::path& executableDirectory) {
    const auto fail = [&](const char* text) {
        error_ = text;
        if (lease_ != INVALID_HANDLE_VALUE) { CloseHandle(lease_); lease_ = INVALID_HANDLE_VALUE; }
        root_.clear();
        return false;
    };
    if (Active() || lease_ != INVALID_HANDLE_VALUE) return fail("Acceptance profile configured more than once.");
    const bool requested = std::any_of(args.begin(), args.end(), [](auto a) { return a.starts_with(L"--acceptance"); });
    if (!enabled) return !requested || fail("Acceptance profiles require an explicit development build.");
    if (!args.empty() && (args.size() != 2 || args[0] != L"--acceptance-root"))
        return fail("Development acceptance builds accept only --acceptance-root and one isolated state path.");
    // Supported desktop launch APIs do not all expose command-line arguments.
    // A no-argument launch still requires the same dedicated package and exact
    // state child below; it can never fall through to ordinary user data.
    const fs::path requestedRoot=args.empty()?executableDirectory/L"state":fs::path(args[1]);
    std::error_code ec;
    if (!reports.is_absolute() || reports.lexically_normal() != reports || !PlainDirectory(reports))
        return fail("Acceptance reports base must be an absolute plain directory.");
    const auto canonicalReports=fs::canonical(reports,ec);
    if(ec)return fail("Cannot resolve acceptance reports base.");
    if (!executableDirectory.is_absolute() || executableDirectory.lexically_normal() != executableDirectory ||
        !PlainDirectory(executableDirectory))return fail("Acceptance package must be an absolute plain directory.");
    const auto package=fs::canonical(executableDirectory,ec);
    if(ec)return fail("Cannot resolve acceptance package.");
    const auto name=package.filename().wstring();
    // Windows TEMP and launch paths can use DOS short names or different drive
    // letter case. Bind the actual canonical directories, not their spelling.
    if (package.parent_path() != canonicalReports ||
        !name.starts_with(L"horde-windows-acceptance-") || name.size() > 96 ||
        name.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-") != name.npos)
        return fail("Acceptance package must be a dedicated direct child of checkout reports.");
    if (!requestedRoot.is_absolute() || requestedRoot.lexically_normal() != requestedRoot ||
        requestedRoot.native().starts_with(L"\\\\") || requestedRoot.filename() != L"state" ||
        !PlainDirectory(requestedRoot.parent_path()) || fs::canonical(requestedRoot.parent_path(),ec) != package || ec)
        return fail("Acceptance state must be exactly the package's ordinary absolute state child.");
    const auto root=package/L"state";
    const bool exists = fs::exists(root, ec);
    if (ec || (exists && (!PlainDirectory(root) || !PlainTree(root))))
        return fail("Acceptance state contains inaccessible, linked or nonregular content.");
    if (!exists && !fs::create_directory(root, ec)) return fail("Cannot create acceptance state directory.");
    const bool empty = fs::is_empty(root, ec);
    if (ec) return fail("Cannot inspect acceptance state directory.");
    lease_ = CreateFileW((root / L"profile.owner").c_str(), GENERIC_READ | (empty ? GENERIC_WRITE : 0),
        0, nullptr, empty ? CREATE_NEW : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (lease_ == INVALID_HANDLE_VALUE) return fail("Acceptance profile is not owned, inaccessible or already in use.");
    DWORD done = 0;
    if (empty) {
        if (!WriteFile(lease_, marker.data(), static_cast<DWORD>(marker.size()), &done, nullptr) ||
            done != marker.size() || !FlushFileBuffers(lease_)) return fail("Cannot establish acceptance profile ownership.");
    } else {
        std::array<char, 128> bytes{};
        if (!ReadFile(lease_, bytes.data(), static_cast<DWORD>(bytes.size()), &done, nullptr) ||
            std::string_view(bytes.data(), done) != marker) return fail("Acceptance profile ownership marker is invalid.");
    }
    const auto campaign = root / L"campaign-v1";
    if (!fs::exists(campaign, ec) && !fs::create_directory(campaign, ec)) return fail("Cannot create isolated campaign directory.");
    if (ec || !PlainDirectory(campaign)) return fail("Isolated campaign directory is not a plain directory.");
    root_ = root;
    return true;
}
}
