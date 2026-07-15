#include "platform/windows/DiagnosticWindow.h"

#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int showCommand)
{
    return horde::platform::windows::RunDiagnosticWindow(showCommand);
}
