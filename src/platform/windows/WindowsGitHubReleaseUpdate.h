#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace horde::platform::windows
{
// The UI thread owns both calls. The worker never downloads or executes a
// release asset; it can only return a release page already verified by the
// shared updater policy.
void BeginGitHubReleaseUpdateCheck(HWND window, const char* installedVersion, bool manualRequest);
bool HandleGitHubReleaseUpdateMessage(HWND window, UINT message, WPARAM token);
void CancelGitHubReleaseUpdateCheck(HWND window);
}
