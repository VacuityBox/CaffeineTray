#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "CaffeineTray.hpp"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Caffeine::CaffeineTray caffe;

    if (!caffe.Init(hInstance))
    {
        MessageBoxW(
            0,
            L"Couldn't initialize Caffeine.\nCheck CaffeineTray.log for more information.",
            L"Initialization failed",
            MB_OK
        );
        return 1;
    }

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
