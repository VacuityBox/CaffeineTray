#include "CaffeineTray.hpp"

#include <shellapi.h>
#include <Psapi.h>
#include <array>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include "resource.h"

namespace Caffeine {

CaffeineTray::CaffeineTray()
    : mWndHandle(0)
    , mInstance(nullptr)
    , mEnumWindowsRetCode(false)
    , mIsTimerRunning(false)
    , mLogger(L"CaffeineTray.log")
    , Settings()
{
    Log() << "---- Log started ----" << std::endl;
}

CaffeineTray::~CaffeineTray()
{
    DisableCaffeine();
    DeleteNotifyIcon();
    UnregisterClassW(L"CaffeineTray_WndClass", mInstance);

    Log() << "---- Log ended ----" << std::endl;
}

auto CaffeineTray::Init(HINSTANCE hInstance) -> bool
{
    Log() << "Initializing Caffeine..." << std::endl;

    if (!hInstance)
    {
        Log() << "hInstance can't be null" << std::endl;
        return false;
    }

    mInstance = hInstance;

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
            Log() << "Failed to register class" << std::endl;
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
            Log() << "Failed to create window";
            return false;
        }

        // Store a pointer to Caffeine object, so we can use it in WinProc.
        SetWindowLongPtrW(mWndHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        //ShowWindow(mHandle, SW_SHOW);
        UpdateWindow(mWndHandle);
    }

    // Create notification icon.
    {
        if (!AddNotifyIcon())
        {
            return false;
        }
    }

    // Load settings.
    {
        LoadSettings();
    }

    // Update icons, timer, power settings.
    {
        UpdateCaffeine();
    }

    Log() << "Initialization finished" << std::endl;

    return true;
}

auto CaffeineTray::UpdateCaffeine() -> void
{
    Log() << "Setting mode to " << ModeToString(Settings.mode) << std::endl;

    switch (Settings.mode)
    {
    case Mode::Disabled:
    case Mode::Auto_Inactive:
        DisableCaffeine();
        break;

    case Mode::Enabled:
    case Mode::Auto_Active:
        EnableCaffeine();
        break;
    }

    ResetTimer();
    UpdateNotifyIcon();
}

auto CaffeineTray::EnableCaffeine() -> void
{
    bool keepDispalyOn = false;
    switch (Settings.mode)
    {
    case Mode::Disabled:
    case Mode::Enabled:
        keepDispalyOn = Settings.Standard.keepDisplayOn;
        break;

    case Mode::Auto_Inactive:
    case Mode::Auto_Active:
        keepDispalyOn = Settings.Auto.keepDisplayOn;
        break;
    }

    auto flags = ES_CONTINUOUS | ES_SYSTEM_REQUIRED;
    if (keepDispalyOn)
    {
        flags |= ES_DISPLAY_REQUIRED;
    }

    SetThreadExecutionState(flags);

    Log() << "Caffeine Enabled. Preventing computer to sleep" << std::endl;
}

auto CaffeineTray::DisableCaffeine() -> void
{
    SetThreadExecutionState(ES_CONTINUOUS);

    Log() << "Caffeine Disabled. Allowing computer to sleep" << std::endl;
}

auto CaffeineTray::ToggleCaffeine() -> void
{
    switch (Settings.mode)
    {
    case Mode::Disabled:
        Settings.mode = Mode::Enabled;
        break;
    case Mode::Enabled:
        Settings.mode = Mode::Auto_Inactive;
        break;
    case Mode::Auto_Inactive:
    case Mode::Auto_Active:
        Settings.mode = Mode::Disabled;
        break;
    }

    UpdateCaffeine();
}

auto CaffeineTray::SetCaffeineMode(Mode mode) -> void
{
    Settings.mode = mode;

    UpdateCaffeine();
}

auto CaffeineTray::AddNotifyIcon() -> bool
{
    NOTIFYICONDATA nid   = { 0 };
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = mWndHandle;
    nid.uFlags           = NIF_GUID | NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.guidItem         = __uuidof(CaffeineNotifyIcon);
    nid.hIcon            = LoadIconHelper(IDI_NOTIFY_CAFFEINE_DISABLED);
    nid.uCallbackMessage = WM_APP_NOTIFY;
    nid.uVersion         = NOTIFYICON_VERSION_4;

    LoadStringW(mInstance, IDS_CAFFEINE_DISABLED, nid.szTip, ARRAYSIZE(nid.szTip));

    if (!Shell_NotifyIconW(NIM_ADD, &nid))
    {
        Log() << "Failed to add notification icon" << std::endl;
        return false;
    }

    if (!Shell_NotifyIconW(NIM_SETVERSION, &nid))
    {
        Log() << "Failed to set version of notification icon" << std::endl;
        return false;
    }

    Log() << "Added notify icon" << std::endl;
    return true;
}

auto CaffeineTray::DeleteNotifyIcon() -> bool
{
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize         = sizeof(nid);
    nid.uFlags         = NIF_GUID;
    nid.guidItem       = __uuidof(CaffeineNotifyIcon);

    if (!Shell_NotifyIconW(NIM_DELETE, &nid))
    {
        Log() << "Failed to delete notification icon" << std::endl;
        return false;
    }

    Log() << "Deleted notify icon" << std::endl;
    return true;
}

auto CaffeineTray::UpdateNotifyIcon() -> bool
{
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize         = sizeof(nid);
    nid.uFlags         = NIF_GUID | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    nid.guidItem       = __uuidof(CaffeineNotifyIcon);

    switch (Settings.mode)
    {
    case Mode::Disabled:
        nid.hIcon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_DISABLED);
        LoadStringW(mInstance, IDS_CAFFEINE_DISABLED, nid.szTip, ARRAYSIZE(nid.szTip));
        break;
    case Mode::Enabled:
        nid.hIcon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_ENABLED);
        LoadStringW(mInstance, IDS_CAFFEINE_ENABLED, nid.szTip, ARRAYSIZE(nid.szTip));
        break;
    case Mode::Auto_Inactive:
        nid.hIcon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE);
        LoadStringW(mInstance, IDS_CAFFEINE_AUTO_INACTIVE, nid.szTip, ARRAYSIZE(nid.szTip));
        break;
    case Mode::Auto_Active:
        nid.hIcon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE);
        LoadStringW(mInstance, IDS_CAFFEINE_AUTO_ACTIVE, nid.szTip, ARRAYSIZE(nid.szTip));
        break;
    }

    if (!Shell_NotifyIconW(NIM_MODIFY, &nid))
    {
        Log() << "Failed to update notification icon" << std::endl;
        return false;
    }

    Log() << "Updated notify icon" << std::endl;
    return true;
}

auto CaffeineTray::LoadIconHelper(WORD icon) -> HICON
{
    const UINT flags = LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED;

    return static_cast<HICON>(
        LoadImageW(mInstance, MAKEINTRESOURCEW(icon), IMAGE_ICON, 0, 0, flags)
        );
}

auto CaffeineTray::ShowNotifyContexMenu(HWND hWnd, LONG x, LONG y) -> void
{
    auto hMenu = static_cast<HMENU>(0);
    switch (Settings.mode)
    {
    case Mode::Disabled:
        hMenu = LoadMenuW(mInstance, MAKEINTRESOURCE(IDC_CAFFEINE_DISABLED_CONTEXTMENU));
        break;
    case Mode::Enabled:
        hMenu = LoadMenuW(mInstance, MAKEINTRESOURCE(IDC_CAFFEINE_ENABLED_CONTEXTMENU));
        break;
    case Mode::Auto_Inactive:
    case Mode::Auto_Active:
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

auto CaffeineTray::LoadSettings() -> void
{
    // Global settings.
    Settings.mode = Mode::Disabled;
    Settings.logLevel = 4;

    // Standard mode settings.
    {
        Settings.Standard.keepDisplayOn = true;
    }

    // Auto mode settings.
    {
        Settings.Auto.keepDisplayOn = true;
        Settings.Auto.updateInterval = 1000;

        Settings.Auto.processPaths.clear();
        Settings.Auto.processNames.clear();
        Settings.Auto.windowTitles.clear();

        // TODO remove this
        //Settings.Auto.processNames.push_back(L"cmd.exe");
        Settings.Auto.windowTitles.push_back(L"Command Prompt");
    }

    Log() << "Loaded settings" << std::endl;
}

auto CaffeineTray::LaunchSettingsProgram() -> bool
{
    return false;
}

auto CaffeineTray::ResetTimer() -> bool
{
    switch (Settings.mode)
    {
    case Mode::Disabled:
    case Mode::Enabled:
        if (mIsTimerRunning)
        {
            KillTimer(mWndHandle, IDT_CAFFEINE);
            mIsTimerRunning = false;
            Log() << "Killing timer" << std::endl;
        }
        break;

    case Mode::Auto_Inactive:
        if (!mIsTimerRunning)
        {
            SetTimer(mWndHandle, IDT_CAFFEINE, Settings.Auto.updateInterval, nullptr);
            mIsTimerRunning = true;
            Log() << "Starting timer" << std::endl;
        }
        break;

    case Mode::Auto_Active:
        break;
    }

    return true;
}

auto CaffeineTray::TimerUpdateProc() -> void
{
    // Activate auto mode if process/window is found. Deactivate otherwise.
    if (ScanProcesses() || ScanWindows())
    {
        // Activate only if inactive.
        if (Settings.mode == Mode::Auto_Inactive)
        {
            Log() << "Found process/window. Activating caffeine." << std::endl;
            SetCaffeineMode(Mode::Auto_Active);
        }
    }
    else
    {
        // Dectivate only if active.
        if (Settings.mode == Mode::Auto_Active)
        {
            Log() << "Process/window no longer exists. Deactivating caffeine." << std::endl;
            SetCaffeineMode(Mode::Auto_Inactive);
        }
    }
}

auto CaffeineTray::ScanProcesses() -> bool
{
    // Get the list of process identifiers (PID's).
    const auto PROCESS_BUFFER_SIZE = 1024;
    auto processList = std::array<DWORD, PROCESS_BUFFER_SIZE>{ 0 };
    auto bytesReturned = static_cast<DWORD>(0);
    if (!EnumProcesses(processList.data(), static_cast<DWORD>(processList.size()), &bytesReturned))
    {
        Log() << "EnumProcesses() failed" << std::endl;
        return false;
    }
    auto numberOfProccesses = bytesReturned / sizeof(DWORD);

    // Loop through running processes.
    for (auto i = 0; i < numberOfProccesses; ++i)
    {
        auto pid = processList[i];
        if (pid != 0)
        {
            auto processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (!processHandle)
            {
                continue;
            }

            // Read process executable path.
            auto imageName = std::array<wchar_t, MAX_PATH>{ 0 };
            auto size = static_cast<DWORD>(MAX_PATH);
            if (QueryFullProcessImageNameW(processHandle, 0, imageName.data(), &size))
            {
                // Check if process is running.
                if (CheckProcess(imageName.data()))
                {
                    CloseHandle(processHandle);
                    return true;
                }
            }
            
            CloseHandle(processHandle);
        }
    }

    return false;
}

auto CaffeineTray::ScanWindows() -> bool
{
    mEnumWindowsRetCode = false;

    if (!EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this)))
    {
        Log() << "EnumWindows() failed" << std::endl;
        return false;
    }

    return mEnumWindowsRetCode;
}

auto CaffeineTray::CheckProcess(const std::wstring_view processImageName) -> bool
{
    // Check if process is on process names list.
    for (auto procName : Settings.Auto.processNames)
    {
        if (processImageName.find(procName) != std::wstring::npos)
        {
            //Log() << "Found running process: '" << procName << "'" << std::endl;
            return true;
        }
    }

    // Check if process is on process paths list.
    for (auto procPath : Settings.Auto.processPaths)
    {
        if (processImageName == procPath)
        {
            //Log() << "Found running process: '" << procPath << "'" << std::endl;
            return true;
        }
    }

    return false;
}

auto CaffeineTray::CheckWindow(const std::wstring_view windowTitle) -> bool
{
    for (auto title : Settings.Auto.windowTitles)
    {
        if (windowTitle.find(title) != std::wstring::npos)
        {
            //Log() << "Found window: '" << title << "'" << std::endl;
            return true;
        }
    }

    return false;
}

auto CaffeineTray::Log() -> std::wostream&
{
    auto& stream = mLogger.good() ? mLogger : std::wcerr;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    localtime_s(&buf, &time);

    stream << "[" << std::put_time(&buf, L"%F %T") << "] ";

    return stream;
}

auto CaffeineTray::ModeToString(Mode mode) -> std::wstring_view
{
    switch (Settings.mode)
    {
    case Mode::Disabled:      return L"Disabled";
    case Mode::Enabled:       return L"Enabled";
    case Mode::Auto_Inactive: return L"Auto (inactive)";
    case Mode::Auto_Active:   return L"Auto (active)";
    }

    return L"Unknown mode";
}

auto CaffeineTray::EnumWindowsProc(HWND hWnd, LPARAM lParam) -> BOOL
{
    auto caffeinePtr = reinterpret_cast<CaffeineTray*>(lParam);
    if (!caffeinePtr)
    {
        return TRUE;
    }

    // NOTE: EnumWindowsProc could return 0 when window is found to stop iterating,
    //       but then EnumWindows would also return 0, so instead: check the flag,
    //       skip the rest and if EnumWindows actually fail it will return 0.
    if (caffeinePtr->mEnumWindowsRetCode)
    {
        return TRUE;
    }

    auto length = GetWindowTextLengthW(hWnd);
    if (length > 0)
    {
        const auto size = length + 1;
        auto title = std::vector(static_cast<size_t>(size) + 1, L'\0');
        GetWindowTextW(hWnd, title.data(), size);
        std::wstring_view sv = title.data();
        if (caffeinePtr->CheckWindow(title.data()))
        {
            caffeinePtr->mEnumWindowsRetCode = true;
            return TRUE;
        }
    }

    return TRUE;
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

        switch (id)
        {
        case IDM_TOGGLE_CAFFEINE:
            caffeinePtr->ToggleCaffeine();
            break;

        case IDM_DISABLE_CAFFEINE:
            caffeinePtr->SetCaffeineMode(Mode::Disabled);
            break;

        case IDM_ENABLE_CAFFEINE:
            caffeinePtr->SetCaffeineMode(Mode::Enabled);
            break;

        case IDM_ENABLE_AUTO:
            caffeinePtr->SetCaffeineMode(Mode::Auto_Inactive);
            break;

        case IDM_SETTINGS:
            caffeinePtr->LaunchSettingsProgram();
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        }

        return 0;
    }
    
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);

        return 0;
    }

    case WM_DESTROY:
    {
        if (caffeinePtr->mIsTimerRunning)
        {
            KillTimer(hWnd, IDT_CAFFEINE);
            caffeinePtr->mIsTimerRunning = false;
        }
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
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

} // namespace Caffeine
