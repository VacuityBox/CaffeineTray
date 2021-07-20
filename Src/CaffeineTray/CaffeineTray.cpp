#include "CaffeineTray.hpp"

#include <shellapi.h>
#include <Psapi.h>
#include <commctrl.h>
#include <ShlObj.h>
#include <array>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include "resource.h"
#include "json.hpp"

namespace Caffeine {

CaffeineTray::CaffeineTray()
    : mWndHandle(0)
    , mInstance(nullptr)
    , mEnumWindowsRetCode(false)
    , mIsTimerRunning(false)
    , Settings()
    , mLightTheme(false)
    , mInitialized(false)
    , mReloadEvent(NULL)
    , mReloadThread(NULL)
    , mSettingsFile(L"Caffeine.json")
{
    // Use AppData for settings if can't open local file.
    auto f = std::ofstream(mSettingsFile);
    if (!f.good())
    {
        auto appDataPath = std::array<wchar_t, MAX_PATH>();
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, appDataPath.data())))
        {
            const auto dir = std::wstring(appDataPath.data()) + L"\\Caffeine\\";
            ::CreateDirectoryW(dir.c_str(), NULL);
            mSettingsFile = dir + L"Caffeine.json";

            const auto logPath = dir + L"CaffeineTray.log";
            mLogger = std::wofstream(logPath);
        }
    }
    else
    {
        mLogger = std::wofstream("CaffeineTray.log");
    }

    Log() << "---- Log started ----" << std::endl;
}

CaffeineTray::~CaffeineTray()
{
    if (mReloadThread)
        ::TerminateThread(mReloadThread, 0);

    if (mReloadEvent)
        ::CloseHandle(mReloadEvent);

    if (mInitialized)
    {
        SaveSettings();
        DeleteNotifyIcon();
    }

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

    // Load settings.
    {
        // Create default settings file if not exists.
        {
            auto f = std::ifstream(mSettingsFile);
            if (!f.good())
                SaveSettings();
        }

        LoadSettings();
        mLightTheme = IsLightTheme();
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
            Log() << "Failed to create reload event." << std::endl;
        }
        else
        {
            auto dwThreadId = DWORD{ 0 };
            mReloadThread = ::CreateThread(NULL, 0, ReloadThreadProc, this, 0, &dwThreadId);
            if (mReloadThread == NULL)
            {
                Log() << "Failed to create reload thread." << std::endl;
            }
        }
    }

    mInitialized = true;
    Log() << "Initialization finished" << std::endl;

    return true;
}

auto CaffeineTray::Update() -> void
{
    ResetTimer();
    UpdateNotifyIcon();
}

auto CaffeineTray::EnableCaffeine() -> void
{
    bool keepDisplayOn = false;
    switch (Settings.mode)
    {
    case CaffeineMode::Disabled:
        break;

    case CaffeineMode::Enabled:
        keepDisplayOn = Settings.Standard.keepDisplayOn;
        break;

    case CaffeineMode::Auto:
        keepDisplayOn = Settings.Auto.keepDisplayOn;
        break;
    }

    auto result = mCaffeine.Enable(keepDisplayOn);
    if (result)
    {
        Log() << "Caffeine Enabled. Preventing computer to sleep" << std::endl;
    }
}

auto CaffeineTray::DisableCaffeine() -> void
{
    auto result = mCaffeine.Disable();
    if (result)
    {
        Log() << "Caffeine Disabled. Allowing computer to sleep" << std::endl;
    }
}

auto CaffeineTray::ToggleCaffeine() -> void
{
    switch (Settings.mode)
    {
    case CaffeineMode::Disabled:
        Settings.mode = CaffeineMode::Enabled;
        EnableCaffeine();
        break;
    case CaffeineMode::Enabled:
        Settings.mode = CaffeineMode::Auto;
        DisableCaffeine();
        break;
    case CaffeineMode::Auto:
        Settings.mode = CaffeineMode::Disabled;
        DisableCaffeine();
        break;
    }

    Update();
}

auto CaffeineTray::SetCaffeineMode(CaffeineMode mode) -> void
{
    if (mode == Settings.mode)
    {
        return;
    }
    Settings.mode = mode;

    switch (Settings.mode)
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
    auto nid             = NOTIFYICONDATA{ 0 };
    nid.cbSize           = sizeof(nid);
    nid.uFlags           = NIF_MESSAGE;
    nid.hWnd             = mWndHandle;
    nid.uCallbackMessage = WM_APP_NOTIFY;

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
    auto nid             = NOTIFYICONDATA{ 0 };
    nid.cbSize           = sizeof(nid);
    nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    nid.hWnd             = mWndHandle;
    nid.uCallbackMessage = WM_APP_NOTIFY;

    switch (Settings.mode)
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
        Log() << "Failed to update notification icon" << std::endl;
        return false;
    }

    Log() << "Updated notify icon" << std::endl;
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
    switch (Settings.mode)
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
    // Read settings file.
    auto file = std::ifstream(mSettingsFile);
    if (!file)
    {
        Log() << "Can't open settings file '" << mSettingsFile.c_str() << "' for reading." << std::endl;
        return false;
    }

    // Deserialize.
    auto json = nlohmann::ordered_json();
    try
    {
        file >> json;
    }
    catch (nlohmann::json::parse_error&)
    {
        Log() << "Failed to deserialize json." << std::endl;
        return false;
    }
    
    // Reset current settings.
    Settings = Settings::Settings();

    // Helper reading functions.
    auto get_or_default = [](const nlohmann::json& json, const auto& key, auto def) {
        auto val = json.find(key);
        if (val == json.end())
            return static_cast<decltype(def)>(def);

        return static_cast<decltype(def)>(*val);
    };

    auto read_array = [](const nlohmann::json& json, const auto& key, auto& dest) {
        auto val = json.find(key);
        if (val == json.end() || !val->is_array())
            return;

        std::vector<std::string> arr = json[key];

        for (const auto& str : arr)
        {
            // Convert UTF-8 to UTF-16.
            if (auto utf16 = UTF8ToUTF16(str))
                dest.push_back(utf16.value());
        }
    };

    // Global settings.
    Settings.mode = get_or_default(json, "mode", CaffeineMode::Disabled);

    // Standard mode settings.
    {
        Settings.Standard.keepDisplayOn = get_or_default(json["Standard"], "keepDisplayOn", 0);
    }

    // Auto mode settings.
    {
        Settings.Auto.keepDisplayOn = get_or_default(json["Auto"], "keepDisplayOn", 0);
        Settings.Auto.scanInterval = get_or_default(json["Auto"], "scanInterval", 1000);

        read_array(json["Auto"], "processPaths", Settings.Auto.processPaths);
        read_array(json["Auto"], "processNames", Settings.Auto.processNames);
        read_array(json["Auto"], "windowTitles", Settings.Auto.windowTitles);
    }

    //Log() << json.dump(4).c_str() << std::endl;
    Log() << "Loaded settings '" << mSettingsFile.c_str() << "'" << std::endl;

    return true;
}

auto CaffeineTray::SaveSettings() -> bool
{
    // Open settings file.
    auto file = std::ofstream(mSettingsFile);
    if (!file)
    {
        Log() << "Can't open settings file '" << mSettingsFile.c_str() << "' for writing." << std::endl;
        return false;
    }

    // Helper.
    auto write_array = [](auto& json, const auto& key, const std::vector<std::wstring>& src) {
        auto& dest = json[key] = nlohmann::json::array();
        for (const auto& str : src)
        {
            // Convert UTF-16 to UTF-8.
            if (auto utf8 = UTF16ToUTF8(str))
                dest.push_back(utf8.value());
        }
    };

    // Create json.
    auto json = nlohmann::ordered_json();

    auto mode = static_cast<int>(Settings.mode);
    json["mode"] = mode;

    json["Standard"]["keepDisplayOn"] = Settings.Standard.keepDisplayOn;

    json["Auto"]["scanInterval"] = Settings.Auto.scanInterval;
    json["Auto"]["keepDisplayOn"] = Settings.Auto.keepDisplayOn;
    write_array(json["Auto"], "processNames", Settings.Auto.processNames);
    write_array(json["Auto"], "processPaths", Settings.Auto.processPaths);
    write_array(json["Auto"], "windowTitles", Settings.Auto.windowTitles);

    // Serialize.
    file << std::setw(4) << json;

    Log() << "Saved settings '" << mSettingsFile.c_str() << "'" << std::endl;
    return true;
}

auto CaffeineTray::LaunchSettingsProgram() -> bool
{
    auto ret = ::ShellExecuteW(mWndHandle, L"open", L"CaffeineSettings.exe", nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(ret) > 32;
}

auto CaffeineTray::ReloadSettings() -> void
{
    Log() << "Settings reload triggered" << std::endl;
    auto mode = Settings.mode;
    if (LoadSettings())
    {
        Settings.mode = mode;
        Update();
    }
}

auto CaffeineTray::UTF8ToUTF16(const std::string_view str) -> std::optional<std::wstring>
{
    // Get size.
    auto size = ::MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        nullptr,
        0
    );

    if (size <= 0)
    {
        return std::nullopt;
    }

    // Convert.
    auto utf16 = std::wstring(size, L'\0');
    auto ret = ::MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        utf16.data(),
        static_cast<int>(utf16.size())
    );

    if (ret == 0)
    {
        return std::nullopt;
    }

    return utf16;
}

auto CaffeineTray::UTF16ToUTF8(const std::wstring_view str) -> std::optional<std::string>
{
    // Get size.
    auto size = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size <= 0)
    {
        return std::nullopt;
    }
    
    // Convert.
    auto utf8 = std::string(size, '\0');
    auto ret = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        utf8.data(),
        static_cast<int>(utf8.size()),
        nullptr,
        nullptr
    );

    if (ret == 0)
    {
        return std::nullopt;
    }

    return utf8;
}

auto CaffeineTray::ResetTimer() -> bool
{
    switch (Settings.mode)
    {
    case CaffeineMode::Disabled:
    case CaffeineMode::Enabled:
        if (mIsTimerRunning)
        {
            KillTimer(mWndHandle, IDT_CAFFEINE);
            mIsTimerRunning = false;
            Log() << "Killing timer" << std::endl;
        }
        break;

    case CaffeineMode::Auto:
        if (!mIsTimerRunning)
        {
            SetTimer(mWndHandle, IDT_CAFFEINE, Settings.Auto.scanInterval, nullptr);
            mIsTimerRunning = true;
            Log() << "Starting timer" << std::endl;
        }
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
        if (!mCaffeine.IsActive())
        {
            Log() << "Found process/window. Activating caffeine." << std::endl;
            EnableCaffeine();
            Update();
        }
    }
    else
    {
        // Dectivate only if active.
        if (mCaffeine.IsActive())
        {
            Log() << "Process/window no longer exists. Deactivating caffeine." << std::endl;
            DisableCaffeine();
            Update();
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
    for (auto i = unsigned long{ 0 }; i < numberOfProccesses; ++i)
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

auto CaffeineTray::IsLightTheme() -> bool
{
    auto data = DWORD{ 0 };
    auto dataSize = DWORD{ sizeof(data) };
    auto status = ::RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"SystemUsesLightTheme",
        RRF_RT_DWORD,
        NULL,
        &data,
        &dataSize
    );

    if (status == ERROR_SUCCESS)
        return data;

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

auto CaffeineTray::ModeToString(CaffeineMode mode) -> std::wstring_view
{
    switch (Settings.mode)
    {
    case CaffeineMode::Disabled:    return L"Disabled";
    case CaffeineMode::Enabled:     return L"Enabled";
    case CaffeineMode::Auto:        return L"Auto";
    }

    return L"Unknown mode";
}

auto CaffeineTray::AboutDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR
{
    const auto caffeineIcon = LoadImageW(
        GetModuleHandleW(NULL),
        MAKEINTRESOURCEW(IDI_CAFFEINE_APP),
        IMAGE_ICON,
        48,
        48,
        LR_DEFAULTCOLOR | LR_SHARED
    );

    const auto license = 
        L"This program is free software: you can redistribute it and/or modify\r\n"
        L"it under the terms of the GNU General Public License as published by\r\n"
        L"the Free Software Foundation, either version 3 of the License, or\r\n"
        L"(at your option) any later version.\r\n"
        L"\r\n"
        L"This program is distributed in the hope that it will be useful,\r\n"
        L"but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n"
        L"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\r\n"
        L"GNU General Public License for more details.\r\n"
        L"\r\n"
        L"You should have received a copy of the GNU General Public License\r\n"
        L"along with this program.  If not, see <a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a>\r\n"
    ;

    const auto homepage = L"<a href=\"https://github.com/VacuityBox/Caffeine\">https://github.com/VacuityBox/Caffeine</a>";

    switch (message)
    {
    case WM_INITDIALOG:
        SendDlgItemMessageW(hWnd, IDC_CAFFEINE_LOGO, STM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(caffeineIcon));
        SetDlgItemTextW(hWnd, IDC_LICENSE, license);
        SetDlgItemTextW(hWnd, IDC_HOMEPAGE, homepage);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hWnd, IDOK);
            break;

        case IDCANCEL:
            EndDialog(hWnd, IDOK);
            break;
        }
        
        return TRUE;

    case WM_NOTIFY:
        switch ((reinterpret_cast<LPNMHDR>(lParam))->code)
        {
        case NM_CLICK:
        case NM_RETURN:
        {
            auto link = reinterpret_cast<PNMLINK>(lParam);
            auto item = link->item;
            ShellExecuteW(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
            EndDialog(hWnd, IDOK);
            break;
        }
        }
        return TRUE;

    case WM_CLOSE:
        EndDialog(hWnd, IDOK);
        return TRUE;
    }

    return FALSE;
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
            caffeinePtr->SetCaffeineMode(CaffeineMode::Disabled);
            break;

        case IDM_ENABLE_CAFFEINE:
            caffeinePtr->SetCaffeineMode(CaffeineMode::Enabled);
            break;

        case IDM_ENABLE_AUTO:
            caffeinePtr->SetCaffeineMode(CaffeineMode::Auto);
            break;

        case IDM_SETTINGS:
            caffeinePtr->LaunchSettingsProgram();
            break;

        case IDM_ABOUT:
            DialogBoxW(caffeinePtr->mInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDlgProc);
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

    case WM_WININICHANGE:
    {
        if (lParam)
        {
            auto str = reinterpret_cast<const wchar_t*>(lParam);
            auto sysparam = std::wstring_view(str);
            if (sysparam == L"ImmersiveColorSet")
            {
                caffeinePtr->mLightTheme = caffeinePtr->IsLightTheme();
                caffeinePtr->UpdateNotifyIcon();
            }
        }
    }
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

} // namespace Caffeine
