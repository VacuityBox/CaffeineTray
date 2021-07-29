#include "CaffeineTray.hpp"

#include "Dialogs/AboutDialog.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "Utility.hpp"
#include "Version.hpp"

#include <shellapi.h>
#include <Psapi.h>
#include <commctrl.h>
#include <ShlObj.h>
#include <array>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include "Resource.h"
#include "json.hpp"

namespace Caffeine {

CaffeineTray::CaffeineTray()
    : mSettings           (std::make_shared<Settings>())
    , mWndHandle          (nullptr)
    , mInstance           (nullptr)
    , mEnumWindowsRetCode (false)
    , mIsTimerRunning     (false)
    , mLightTheme         (false)
    , mInitialized        (false)
    , mReloadEvent        (NULL)
    , mReloadThread       (NULL)
    , mProcessScanner     (mSettings)
    , mWindowScanner      (mSettings)
{
    auto appData = GetAppDataPath() / "CaffeineTray";
    fs::create_directory(appData);
    
    mSettingsFilePath = appData / CAFFEINE_SETTINGS_FILENAME;
    mLoggerFilePath = appData / CAFFEINE_LOG_FILENAME;

    mLogger.Open(mLoggerFilePath);
    Log("---- Log started ----");
}

CaffeineTray::~CaffeineTray()
{
    if (mReloadThread)
        ::TerminateThread(mReloadThread, 0);

    if (mReloadEvent)
        ::CloseHandle(mReloadEvent);

    if (mInitialized)
        DeleteNotifyIcon();

    UnregisterClassW(L"CaffeineTray_WndClass", mInstance);

    Log("---- Log ended ----");
}

auto CaffeineTray::Init(HINSTANCE hInstance) -> bool
{
    Log("Initializing Caffeine...");

    if (!hInstance)
    {
        Log("hInstance can't be null");
        return false;
    }

    mInstance = hInstance;

    // Load Settings.
    {
        // Create default settings file if not exists.
        if (!fs::exists(mSettingsFilePath))
        {
            Log("Settings file not found, creating default one");            
            SaveSettings();
        }
        else
        {
            if (!LoadSettings())
            {
                return false;
            }
        }

        mLightTheme = IsLightTheme();
    }

    // Init window class.
    {
        WNDCLASSEXW wcex   = { 0 };
        wcex.cbSize        = sizeof(wcex);
        wcex.style         = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = WndProc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = 0;
        wcex.hInstance     = mInstance;
        wcex.hIcon         = 0; // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CAFFEINE));
        wcex.hIconSm       = 0; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
        wcex.hCursor       = 0; // LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = 0; // (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName  = 0;
        wcex.lpszClassName = L"CaffeineTray_WndClass";

        if (!RegisterClassExW(&wcex))
        {
            Log("Failed to register class");
            return false;
        }
    }

    // Create window.
    {
        mWndHandle = CreateWindowW(
            L"CaffeineTray_WndClass",
            L"CaffeineTray",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            nullptr,
            nullptr,
            mInstance,
            nullptr
        );

        if (!mWndHandle)
        {
            Log("Failed to create window");
            return false;
        }

        // Store a pointer to ExecutionState object, so we can use it in WinProc.
        SetWindowLongPtrW(mWndHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        //ShowWindow(mHandle, SW_SHOW);
        UpdateWindow(mWndHandle);

        // For hyperlinks in About dialog.
        auto ccs = INITCOMMONCONTROLSEX{ 0 };
        ccs.dwSize = sizeof(ccs);
        ccs.dwICC = ICC_LINK_CLASS;
        InitCommonControlsEx(&ccs);
    }

    // Create notification icon.
    {
        if (!AddNotifyIcon())
        {
            return false;
        }
    }

    // Update icons, timer, power settings.
    {
        Update();
    }

    // Create reload event and thread.
    {
        mReloadEvent = ::CreateEventW(NULL, FALSE, FALSE, L"CaffeineTray_ReloadEvent");
        if (mReloadEvent == NULL)
        {
            Log("Failed to create reload event.");
        }
        else
        {
            auto dwThreadId = DWORD{ 0 };
            mReloadThread = ::CreateThread(NULL, 0, ReloadThreadProc, this, 0, &dwThreadId);
            if (mReloadThread == NULL)
            {
                Log("Failed to create reload thread.");
            }
        }
    }

    mInitialized = true;
    Log("Initialization finished");

    return true;
}

auto CaffeineTray::Update() -> void
{
    ResetTimer();
    UpdateNotifyIcon();
}

auto CaffeineTray::EnableCaffeine() -> void
{
    auto keepDisplayOn = false;
    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        break;

    case CaffeineMode::Enabled:
        keepDisplayOn = mSettings->Standard.KeepDisplayOn;
        break;

    case CaffeineMode::Auto:
        keepDisplayOn = mSettings->Auto.KeepDisplayOn;
        break;
    }

    auto result = mCaffeine.PreventSleep(keepDisplayOn);
    if (result)
    {
        Log("Caffeine Enabled. Preventing computer to sleep");
    }
}

auto CaffeineTray::DisableCaffeine() -> void
{
    auto result = mCaffeine.AllowSleep();
    if (result)
    {
        Log("Caffeine Disabled. Allowing computer to sleep");
    }
}

auto CaffeineTray::ToggleCaffeine() -> void
{
    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        mSettings->Mode = CaffeineMode::Enabled;
        EnableCaffeine();
        break;
    case CaffeineMode::Enabled:
        mSettings->Mode = CaffeineMode::Auto;
        DisableCaffeine();
        break;
    case CaffeineMode::Auto:
        mSettings->Mode = CaffeineMode::Disabled;
        DisableCaffeine();
        break;
    }

    Update();
}

auto CaffeineTray::SetCaffeineMode(CaffeineMode mode) -> void
{
    if (mode == mSettings->Mode)
    {
        return;
    }
    mSettings->Mode = mode;

    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        DisableCaffeine();
        break;
    case CaffeineMode::Enabled:
        EnableCaffeine();
        break;
    case CaffeineMode::Auto:
        DisableCaffeine();
        break;
    }

    Update();
}

auto CaffeineTray::AddNotifyIcon() -> bool
{
    auto nid             = NOTIFYICONDATA{ 0 };
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = mWndHandle;
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.hIcon            = LoadIconHelper(IDI_NOTIFY_CAFFEINE_DISABLED);
    nid.hWnd             = mWndHandle;
    nid.uCallbackMessage = WM_APP_NOTIFY;
    nid.uVersion         = NOTIFYICON_VERSION_4;

    LoadStringW(mInstance, IDS_CAFFEINE_DISABLED, nid.szTip, ARRAYSIZE(nid.szTip));

    if (!Shell_NotifyIconW(NIM_ADD, &nid))
    {
        Log("Failed to add notification icon");
        return false;
    }

    if (!Shell_NotifyIconW(NIM_SETVERSION, &nid))
    {
        Log("Failed to set version of notification icon");
        return false;
    }

    Log("Added notify icon");
    return true;
}

auto CaffeineTray::DeleteNotifyIcon() -> bool
{
    auto nid             = NOTIFYICONDATA{ 0 };
    nid.cbSize           = sizeof(nid);
    nid.uFlags           = NIF_MESSAGE;
    nid.hWnd             = mWndHandle;
    nid.uCallbackMessage = WM_APP_NOTIFY;

    if (!Shell_NotifyIconW(NIM_DELETE, &nid))
    {
        Log("Failed to delete notification icon");
        return false;
    }

    Log("Deleted notify icon");
    return true;
}

auto CaffeineTray::UpdateNotifyIcon() -> bool
{
    auto nid             = NOTIFYICONDATA{ 0 };
    nid.cbSize           = sizeof(nid);
    nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    nid.hWnd             = mWndHandle;
    nid.uCallbackMessage = WM_APP_NOTIFY;

    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        nid.hIcon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_DISABLED);
        LoadStringW(mInstance, IDS_CAFFEINE_DISABLED, nid.szTip, ARRAYSIZE(nid.szTip));
        break;
    case CaffeineMode::Enabled:
        nid.hIcon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_ENABLED);
        LoadStringW(mInstance, IDS_CAFFEINE_ENABLED, nid.szTip, ARRAYSIZE(nid.szTip));
        break;
    case CaffeineMode::Auto:
        if (!mCaffeine.IsActive())
        {
            nid.hIcon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE);
            LoadStringW(mInstance, IDS_CAFFEINE_AUTO_INACTIVE, nid.szTip, ARRAYSIZE(nid.szTip));
        }
        else
        {
            nid.hIcon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE);
            LoadStringW(mInstance, IDS_CAFFEINE_AUTO_ACTIVE, nid.szTip, ARRAYSIZE(nid.szTip));
        }
        break;
    }

    if (!Shell_NotifyIconW(NIM_MODIFY, &nid))
    {
        Log("Failed to update notification icon");
        return false;
    }

    Log("Updated notify icon");
    return true;
}

auto CaffeineTray::LoadIconHelper(WORD icon) -> HICON
{
    const auto flags = UINT{ LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED };

    auto id = WORD{ 32512 };
    if (mLightTheme)
    {
        switch (icon)
        {
        case IDI_CAFFEINE_APP:                  id = IDI_CAFFEINE_APP_DARK;                  break;
        case IDI_NOTIFY_CAFFEINE_DISABLED:      id = IDI_NOTIFY_CAFFEINE_DISABLED_DARK;      break;
        case IDI_NOTIFY_CAFFEINE_ENABLED:       id = IDI_NOTIFY_CAFFEINE_ENABLED_DARK;       break;
        case IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE: id = IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE_DARK; break;
        case IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE:   id = IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE_DARK;   break;
        }
    }
    else
    {
        switch (icon)
        {
        case IDI_CAFFEINE_APP:                  id = IDI_CAFFEINE_APP_LIGHT;                  break;
        case IDI_NOTIFY_CAFFEINE_DISABLED:      id = IDI_NOTIFY_CAFFEINE_DISABLED_LIGHT;      break;
        case IDI_NOTIFY_CAFFEINE_ENABLED:       id = IDI_NOTIFY_CAFFEINE_ENABLED_LIGHT;       break;
        case IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE: id = IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE_LIGHT; break;
        case IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE:   id = IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE_LIGHT;   break;
        }
    }
    
    return static_cast<HICON>(
        LoadImageW(mInstance, MAKEINTRESOURCEW(id), IMAGE_ICON, 0, 0, flags)
        );
}

auto CaffeineTray::ShowNotifyContexMenu(HWND hWnd, LONG x, LONG y) -> void
{
    auto hMenu = static_cast<HMENU>(0);
    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        hMenu = LoadMenuW(mInstance, MAKEINTRESOURCE(IDC_CAFFEINE_DISABLED_CONTEXTMENU));
        break;
    case CaffeineMode::Enabled:
        hMenu = LoadMenuW(mInstance, MAKEINTRESOURCE(IDC_CAFFEINE_ENABLED_CONTEXTMENU));
        break;
    case CaffeineMode::Auto:
        hMenu = LoadMenuW(mInstance, MAKEINTRESOURCE(IDC_CAFFEINE_AUTO_CONTEXTMENU));
        break;
    }

    if (hMenu)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu)
        {
            // our window must be foreground before calling TrackPopupMenu
            // or the menu will not disappear when the user clicks away
            SetForegroundWindow(hWnd);

            // respect menu drop alignment
            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
            {
                uFlags |= TPM_RIGHTALIGN;
            }
            else
            {
                uFlags |= TPM_LEFTALIGN;
            }

            TrackPopupMenuEx(hSubMenu, uFlags, x, y, hWnd, NULL);
        }

        DestroyMenu(hMenu);
    }
}

auto CaffeineTray::LoadSettings() -> bool
{
    // NOTE: Settings should be in UTF-8
    // Read Settings file.
    auto file = std::ifstream(mSettingsFilePath);
    if (!file)
    {
        Log("Can't open Settings file '" + mSettingsFilePath.string() + "' for reading");
        return false;
    }

    // Deserialize.
    auto json = nlohmann::json::parse(file, nullptr, false, true);
    if (json.is_discarded())
    {
        Log("Failed to deserialize json");
        return false;
    }
    
    *mSettings = json.get<Settings>();

    //Log() << json.dump(4).c_str() << std::endl;
    Log("Loaded Settings '" + mSettingsFilePath.string() + "'");

    return true;
}

auto CaffeineTray::SaveSettings() -> bool
{
    // Open Settings file.
    auto file = std::ofstream(mSettingsFilePath);
    if (!file)
    {
        Log("Can't open Settings file '" + mSettingsFilePath.string() + "' for writing");
        return false;
    }

    // Serialize.
    auto json = nlohmann::ordered_json(*mSettings);
    file << std::setw(4) << json;

    Log("Saved Settings '" + mSettingsFilePath.string() + "'");
    return true;
}

auto CaffeineTray::ReloadSettings() -> void
{
    Log("Settings reload triggered");
    auto mode = mSettings->Mode;
    if (LoadSettings())
    {
        mSettings->Mode = mode;
        Update();
    }
}

auto CaffeineTray::ResetTimer() -> bool
{
    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
    case CaffeineMode::Enabled:
        if (mIsTimerRunning)
        {
            KillTimer(mWndHandle, IDT_CAFFEINE);
            mIsTimerRunning = false;
            Log("Killing timer");
        }
        break;

    case CaffeineMode::Auto:
        if (!mIsTimerRunning)
        {
            SetTimer(mWndHandle, IDT_CAFFEINE, mSettings->Auto.ScanInterval, nullptr);
            mIsTimerRunning = true;
            Log("Starting timer");
        }
        break;
    }

    return true;
}

auto CaffeineTray::TimerUpdateProc() -> void
{
    // Scan processes and windows if no process found.
    auto foundProcess = mProcessScanner.Run();
    auto foundWindow  = false;
    if (!foundProcess)
    {
        foundWindow = mWindowScanner.Run();
    }

    // Activate auto mode if process/window is found. Deactivate otherwise.
    if (foundProcess || foundWindow)
    {
        // Activate only if inactive.
        if (!mCaffeine.IsActive())
        {
            if (foundProcess)
            {
                auto utf8Name = UTF16ToUTF8(mProcessScanner.GetLastFound());
                if (utf8Name)
                    Log("Found process '" + utf8Name.value() + "'");
                else
                    Log("Found process");
            }
            else
            {
                auto utf8Name = UTF16ToUTF8(mWindowScanner.GetLastFound());
                if (utf8Name)
                    Log("Found window '" + utf8Name.value() + "'");
                else
                    Log("Found window");
            }

            
            Log("Activating Caffeine");
            EnableCaffeine();
            Update();
        }
    }
    else
    {
        // Dectivate only if active.
        if (mCaffeine.IsActive())
        {
            Log("Process/window no longer exists. Deactivating caffeine.");
            DisableCaffeine();
            Update();
        }
    }
}

auto CaffeineTray::Log(std::string message) -> void
{
    mLogger.Log(std::move(message));
}

auto CaffeineTray::ShowCaffeineSettings ()
{
    static auto isCreated = false;
    if (isCreated)
    {
        return false;
    }
    isCreated = true;

    auto caffeineSettings = CaffeineSettings(mSettings);
    if (caffeineSettings.Show(mWndHandle))
    {
        const auto& newSettings = caffeineSettings.Result();

        mSettings->Standard = newSettings.Standard;
        mSettings->Auto     = newSettings.Auto;

        // Update with new settings.
        Update();
    }

    isCreated = false;

    return true;
}

auto CaffeineTray::ShowAboutDialog ()
{
    static auto isCreated = false;
    if (isCreated)
    {
        return false;
    }
    isCreated = true;

    auto aboutDlg = AboutDialog();
    aboutDlg.Show(mWndHandle);

    isCreated = false;

    return true;
}

auto CaffeineTray::ReloadThreadProc(LPVOID lParam) -> DWORD
{
    while (true)
    {
        auto caffeinePtr = reinterpret_cast<CaffeineTray*>(lParam);
        if (!caffeinePtr)
        {
            return 1;
        }

        auto waitRet = ::WaitForSingleObject(caffeinePtr->mReloadEvent, INFINITE);
        switch (waitRet)
        {
        case WAIT_OBJECT_0:
            caffeinePtr->ReloadSettings();
            break;
        }
    }

    return 0;
}

auto CALLBACK CaffeineTray::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
{
    auto caffeinePtr = reinterpret_cast<CaffeineTray*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (!caffeinePtr)
    {
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_COMMAND:
    {
        auto id = LOWORD(wParam);

        switch (LOWORD(wParam))
        {
        case IDM_TOGGLE_CAFFEINE:
            caffeinePtr->ToggleCaffeine();
            break;

        case IDM_DISABLE_CAFFEINE:
            caffeinePtr->SetCaffeineMode(CaffeineMode::Disabled);
            break;

        case IDM_ENABLE_CAFFEINE:
            caffeinePtr->SetCaffeineMode(CaffeineMode::Enabled);
            break;

        case IDM_ENABLE_AUTO:
            caffeinePtr->SetCaffeineMode(CaffeineMode::Auto);
            break;

        case IDM_SETTINGS:
            caffeinePtr->ShowCaffeineSettings();
            break;

        case IDM_ABOUT:
            caffeinePtr->ShowAboutDialog();
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        }

        return 0;
    }

    case WM_DESTROY:
    {
        if (caffeinePtr->mIsTimerRunning)
        {
            KillTimer(hWnd, IDT_CAFFEINE);
            caffeinePtr->mIsTimerRunning = false;
        }

        caffeinePtr->SaveSettings();

        PostQuitMessage(0);
        return 0;
    }

    case WM_APP_NOTIFY:
    {
        switch (LOWORD(lParam))
        {
        case NIN_SELECT:
            caffeinePtr->ToggleCaffeine();
            break;

        case WM_CONTEXTMENU:
            caffeinePtr->ShowNotifyContexMenu(hWnd, LOWORD(wParam), HIWORD(wParam));
            break;
        }

        return 0;
    }

    case WM_TIMER:
    {
        switch (wParam)
        {
        case IDT_CAFFEINE:
            caffeinePtr->TimerUpdateProc();
            break;
        }
        return 0;
    }

    case WM_WININICHANGE:
    {
        if (lParam)
        {
            auto str = reinterpret_cast<const wchar_t*>(lParam);
            auto sysparam = std::wstring_view(str);
            if (sysparam == L"ImmersiveColorSet")
            {
                caffeinePtr->mLightTheme = IsLightTheme();
                caffeinePtr->UpdateNotifyIcon();
            }
        }

        return 0;
    }
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

} // namespace Caffeine
