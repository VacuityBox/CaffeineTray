#include "NotifyIcon.hpp"

#include <shellapi.h>
#include <strsafe.h>

namespace Caffeine {

auto NotifyIcon::Dispatch (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
{
    // Return 0 when handling message, DefWindowProc otherwise.
    switch (message)
    {
    case WM_CREATE:
        if (OnCreate())
        {
            return 0;
        }
        break;

    case WM_COMMAND:
        if (OnCommand(wParam, lParam))
        {
            return 0;
        }
        break;

    case WM_APP_NOTIFY:
        switch (LOWORD(lParam))
        {
        case NIN_SELECT:
            if (OnClick(wParam, lParam))
            {
                return 0;
            }
            break;

        case WM_CONTEXTMENU:
            if (OnContextMenu(wParam, lParam))
            {
                return 0;
            }
            break;
        }

        return 0;

    case WM_DESTROY:
        OnDestroy();
        DeleteNotifyIcon();
        PostQuitMessage(0);
        return 0;        
    }
    
    return CustomDispatch(hWnd, message, wParam, lParam);
}

auto NotifyIcon::CustomDispatch (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}

auto CALLBACK NotifyIcon::NotifyIconProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
{
    auto notifyIconPtr = static_cast<NotifyIcon*>(nullptr);

    if (message == WM_NCCREATE)
    {
        auto lpCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);

        notifyIconPtr = reinterpret_cast<NotifyIcon*>(lpCreateStruct->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(notifyIconPtr));
        
        notifyIconPtr->mWindowHandle = hWnd;
    }
    else
    {
        notifyIconPtr = reinterpret_cast<NotifyIcon*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (notifyIconPtr)
    {
        return notifyIconPtr->Dispatch(hWnd, message, wParam, lParam);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

auto NotifyIcon::CreateNotifyIcon () -> int
{
    if (mInstanceHandle == NULL)
    {
        if (!(mInstanceHandle = GetModuleHandle(NULL)))
        {
            return 1;
        }
    }

    // Init window class.
    {
        WNDCLASSEXW wcex   = { 0 };
        wcex.cbSize        = sizeof(wcex);
        wcex.style         = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = NotifyIconProc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = 0;
        wcex.hInstance     = mInstanceHandle;
        wcex.hIcon         = 0; // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CAFFEINE));
        wcex.hIconSm       = 0; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
        wcex.hCursor       = 0; // LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = 0; // (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName  = 0;
        wcex.lpszClassName = NOTIFYICON_WNDCLASS;

        if (!RegisterClassEx(&wcex))
        {
            return 2;
        }
    }

    // Create window.
    {
        mWindowHandle = CreateWindowW(
            NOTIFYICON_WNDCLASS,
            L"NotifyIcon",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            nullptr,
            nullptr,
            mInstanceHandle,
            this
        );

        if (!mWindowHandle)
        {
            return 3;
        }

        //ShowWindow(mWindowHandle, SW_SHOW);
        UpdateWindow(mWindowHandle);
    }

    // Add notification icon.
    {
        if (!AddNotifyIcon())
        {
            return 4;
        }
    }

    return 0;
}


auto NotifyIcon::AddNotifyIcon () -> bool
{
    auto nid             = NOTIFYICONDATA{ 0 };
    nid.cbSize           = sizeof(nid);
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.hWnd             = mWindowHandle;
    nid.uCallbackMessage = WM_APP_NOTIFY;
    nid.hIcon            = LoadIcon(NULL, IDI_ASTERISK);
    nid.uVersion         = NOTIFYICON_VERSION_4;

    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), L"NotifyIcon Tip");

    if (!Shell_NotifyIconW(NIM_ADD, &nid))
    {
        return false;
    }

    if (!Shell_NotifyIconW(NIM_SETVERSION, &nid))
    {        
        return false;
    }

    return true;
}

auto NotifyIcon::UpdateNotifyIcon (HICON icon, UINT tipId) -> bool
{
    auto nid             = NOTIFYICONDATA{ 0 };
    nid.cbSize           = sizeof(nid);
    nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    nid.hWnd             = mWindowHandle;
    nid.uCallbackMessage = WM_APP_NOTIFY;
    nid.hIcon            = icon;
    
    LoadStringW(mInstanceHandle, tipId, nid.szTip, ARRAYSIZE(nid.szTip));

    return Shell_NotifyIconW(NIM_MODIFY, &nid);
}

auto NotifyIcon::DeleteNotifyIcon () -> bool
{
    auto nid             = NOTIFYICONDATA{ 0 };
    nid.cbSize           = sizeof(nid);
    nid.uFlags           = NIF_MESSAGE;
    nid.hWnd             = mWindowHandle;
    nid.uCallbackMessage = WM_APP_NOTIFY;

    return Shell_NotifyIconW(NIM_DELETE, &nid);
}

auto NotifyIcon::MainLoop () -> WPARAM
{
    // Main message loop.
    auto msg = MSG{0};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!IsWindow(mWindowHandle) || !IsDialogMessage(mWindowHandle, &msg)) 
        {
            TranslateMessage(&msg); 
            DispatchMessage(&msg);
        }
    }

    mWindowHandle = NULL;

    return msg.wParam;
}

} // namespace Caffeine
