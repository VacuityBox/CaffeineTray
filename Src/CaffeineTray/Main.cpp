#include "AppInitInfo.hpp"
#include "CaffeineTray.hpp"
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
    auto guard = Caffeine::InstanceGuard();
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

    const auto info = Caffeine::GetAppInitInfo(hInstance);

    Caffeine::InitLogger(info.LogFilePath);

    auto caffeineTray = Caffeine::CaffeineTray(info);
    if (!caffeineTray.Init())
    {
        MessageBoxW(
            0,
            L"Failed to initialize CaffeineTray.\nCheck CaffeineTray.log for more information.",
            L"Initialization failed",
            MB_OK
        );
        return -2;
    }

    return static_cast<int>(caffeineTray.MainLoop());
}
