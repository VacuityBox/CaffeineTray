#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "CaffeineTray.hpp"
#include "InstanceGuard.hpp"
#include "Logger.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    Caffeine::InitLogger();

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

    auto caffeineTray = Caffeine::CaffeineTray(hInstance);
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
