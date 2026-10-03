#include "platform/windows/WindowsMusicFocus.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main()
{
    using horde::platform::windows::WindowsMusicShouldSuspend;
    for (unsigned bits = 0; bits != 16; ++bits)
    {
        const bool foreground = (bits & 1) != 0;
        const bool ready = (bits & 2) != 0;
        const bool menuPaused = (bits & 4) != 0;
        const bool explicitLoss = (bits & 8) != 0;
        const bool expected = !foreground || !ready || menuPaused || explicitLoss;
        if (WindowsMusicShouldSuspend(foreground, ready, menuPaused, explicitLoss) != expected)
        {
            std::cerr << "Windows music focus gate failed for combination " << bits << '\n';
            return 1;
        }
    }
    // Observed control: the actual window was foreground during gameplay but
    // its latched app-active flag was false. No cached flag is an input anymore.
    if (WindowsMusicShouldSuspend(true, true, false) ||
        !WindowsMusicShouldSuspend(true, true, false, true)) return 2;

    std::ifstream source("src/platform/windows/DiagnosticWindow.cpp", std::ios::binary);
    std::ostringstream text;
    text << source.rdbuf();
    const std::string window = text.str();
    if (!source || window.find("musicWindowActive") != std::string::npos ||
        window.find("GetForegroundWindow() == context.windowHandle") == std::string::npos ||
        window.find("WindowsMusicShouldSuspend(") == std::string::npos ||
        window.find("PublishMusicPlayback(*sceneContext, wParam == FALSE)") == std::string::npos ||
        window.find("PublishMusicPlayback(*sceneContext, LOWORD(wParam) == WA_INACTIVE)") == std::string::npos)
    {
        std::cerr << "Windows publisher must sample foreground and honor immediate app/window loss\n";
        return 3;
    }
    std::cout << "PASS: 16 focus/ready/menu/loss combinations and actual-window publication wiring\n";
    return 0;
}
