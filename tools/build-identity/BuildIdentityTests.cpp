#include "reporting/BuildIdentity.h"

#include <iostream>
#include <string>
#include <string_view>

using horde::reporting::IsBuildIdentityReportLabel;
constexpr std::string_view kUnavailable = "g:unavailable;i:unavailable;c:U;s:U";
static_assert(IsBuildIdentityReportLabel(kUnavailable));
static_assert(!IsBuildIdentityReportLabel("g:unavailable;i:unavailable;c:U;s:C"));
static_assert(!IsBuildIdentityReportLabel("g:unavailable;i:unavailable;c:Debug;s:U"));

int main()
{
    const auto require = [](const bool condition, const char* reason)
    {
        if (!condition) std::cerr << "FAIL: " << reason << '\n';
        return condition;
    };
    const std::string commit(40u, '1');
    const std::string digest(64u, '2'); // Numeric runs must pass the typed grammar.
    const std::string valid = "g:" + commit + ";i:" + digest + ";c:RD;s:D";
    bool ok = require(valid.size() == 118u && IsBuildIdentityReportLabel(valid), "bounded full identity");
    for (const std::string_view config : {"D", "R", "RD", "M", "U"})
        for (const std::string_view state : {"C", "D", "U"})
            ok &= require(IsBuildIdentityReportLabel("g:" + commit + ";i:" + digest + ";c:" +
                std::string(config) + ";s:" + std::string(state)), "all exact config/state tokens");
    for (const std::string bad : {valid + " ", valid + ";extra:123", " " + valid,
        "g:" + std::string(39u, '1') + ";i:" + digest + ";c:R;s:C",
        "g:" + std::string(40u, 'A') + ";i:" + digest + ";c:R;s:C",
        "g:" + commit + ";i:" + std::string(63u, '2') + ";c:R;s:C",
        "g:" + commit + ";i:" + std::string(64u, 'G') + ";c:R;s:C",
        "g:" + commit + ";i:" + digest + ";c:Debug;s:C",
        "g:" + commit + ";i:" + digest + ";c:R;s:clean",
        "g:unavailable;i:" + digest + ";c:R;s:D",
        "g:" + commit + ";i:unavailable;c:R;s:C",
        "g:user@example.com;i:" + digest + ";c:R;s:U",
        std::string("g:unavailable;i:unavailable;c:C:/Users/private;s:U")})
        ok &= require(!IsBuildIdentityReportLabel(bad), "reject malformed/freeform identity");
    std::string embedded = valid;
    embedded[10] = '\0';
    ok &= require(!IsBuildIdentityReportLabel(embedded), "reject embedded NUL");
    const auto identity = horde::reporting::CurrentBuildIdentity();
    ok &= require(IsBuildIdentityReportLabel(identity.ReportLabel()), "current stamp follows grammar");
#if HORDE_RT_TEST_EXPECT_STAMP
    ok &= require(identity.sourceCommit.size() == 40u && identity.nativeInputSha256.size() == 64u,
        "attached target embeds refreshed stamp");
#else
    ok &= require(identity.ReportLabel() == kUnavailable && identity.inputState == "unavailable",
        "unattached target explicitly unavailable");
#endif
    if (!ok) return 1;
    std::cout << identity.ReportLabel() << '\n';
    return 0;
}
