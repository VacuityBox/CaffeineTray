#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "CaffeineTray.hpp"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Check if application is not running already.
    auto mutex = ::CreateMutexW(NULL, FALSE, L"CaffeineTray_AppInstance");
    if (mutex == NULL)
    {
        MessageBoxW(
            0,
            L"Failed to create mutex.",
            L"Initialization failed",
            MB_OK
        );
        return -1;
    }

    auto caffe = Caffeine::CaffeineTray();
    if (!caffe.Init(hInstance))
    {
        MessageBoxW(
            0,
            L"Couldn't initialize Caffeine.\nCheck CaffeineTray.log for more information.",
            L"Initialization failed",
            MB_OK
        );
        return -2;
    }

    // Main message loop:
    auto msg = MSG{ 0 };
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(mutex);

    return static_cast<int>(msg.wParam);
}
