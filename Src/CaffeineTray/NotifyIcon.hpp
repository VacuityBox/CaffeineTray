#pragma once

#include "Logger.hpp"

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Caffeine {

class NotifyIcon
{
protected:
    static constexpr auto WM_APP_NOTIFY       = UINT{WM_APP + 1}; // NotifyIcon messages
    static constexpr auto NOTIFYICON_WNDCLASS = L"NotifyIcon_WndClass";

    HWND      mWindowHandle;
    HINSTANCE mInstanceHandle;

    auto CreateNotifyIcon () -> int;
    auto UpdateNotifyIcon (HICON icon, UINT tip) -> bool;

private:
    auto AddNotifyIcon    () -> bool;
    auto DeleteNotifyIcon () -> bool;

    // Return true when message is handled.
    virtual auto OnCreate      () -> bool = 0;
    virtual auto OnDestroy     () -> void = 0;
    virtual auto OnCommand     (WPARAM, LPARAM) -> bool = 0;
    virtual auto OnClick       (WPARAM, LPARAM) -> bool = 0;
    virtual auto OnContextMenu (WPARAM, LPARAM) -> bool = 0;

    auto Dispatch (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT;

    // Override to handle other messages.
    virtual auto CustomDispatch (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT = 0;

    static auto CALLBACK NotifyIconProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT;

    NotifyIcon (const NotifyIcon& rhs) = delete;
    NotifyIcon (NotifyIcon&& rhs) = delete;

    NotifyIcon& operator=(const NotifyIcon& rhs) = delete;
    NotifyIcon& operator=(NotifyIcon&& rhs) = delete;

public:
    NotifyIcon (HINSTANCE hInstance)
        : mWindowHandle   (nullptr)
        , mInstanceHandle (hInstance)
    {
    }

    virtual ~NotifyIcon ()
    {
        UnregisterClass(NOTIFYICON_WNDCLASS, mInstanceHandle);
    }

    auto MainLoop  () -> WPARAM;
    auto GetHandle () { return mWindowHandle; };
};

} // namespace Caffeine

