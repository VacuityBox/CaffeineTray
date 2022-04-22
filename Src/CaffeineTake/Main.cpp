#include "AppInitInfo.hpp"
#include "CaffeineTake.hpp"
#include "InstanceGuard.hpp"
#include "Logger.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Check if application is not running already.
    auto guard = CaffeineTake::InstanceGuard();
    if (!guard.Protect())
    {
        MessageBoxW(
            0,
            L"Failed to create instance guard.",
            L"Initialization failed",
            MB_OK
        );
        return -1;
    }

    if (guard.IsSecondInstance())
    {
        return 1;
    }

    const auto info = CaffeineTake::GetAppInitInfo(hInstance);

    CaffeineTake::InitLogger(info.LogFilePath);

    auto caffeineTray = CaffeineTake::CaffeineTakeApp(info);
    if (!caffeineTray.Init())
    {
        MessageBoxW(
            0,
            L"Failed to initialize CaffeineTake.\nCheck CaffeineTake.log for more information.",
            L"Initialization failed",
            MB_OK
        );
        return -2;
    }

    return static_cast<int>(caffeineTray.MainLoop());
}
