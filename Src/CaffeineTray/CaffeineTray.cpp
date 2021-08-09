#include "CaffeineTray.hpp"

#include "Dialogs/AboutDialog.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "json.hpp"
#include "Resource.h"
#include "Utility.hpp"
#include "Version.hpp"

#include <filesystem>
#include <fstream>
#include <commctrl.h>
#include <Psapi.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <WtsApi32.h>

using namespace std;

namespace Caffeine {

CaffeineTray::CaffeineTray(HINSTANCE hInstance)
    : NotifyIcon          (hInstance)
    , mSettings           (std::make_shared<Settings>())
    , mLogger             (std::make_shared<Logger>())
    , mLightTheme         (false)
    , mInitialized        (false)
    , mSettingsChanged    (false)
    , mSessionLocked      (false)
    , mProcessScanner     (mSettings)
    , mWindowScanner      (mSettings)
    , mScannerTimer       (std::bind(&CaffeineTray::TimerUpdate, this))
    , mCaffeine           (mLogger)
{
    // Portable mode.
    if (fs::exists(CAFFEINE_PORTABLE_SETTINGS_FILENAME))
    {
        mSettingsFilePath = CAFFEINE_PORTABLE_SETTINGS_FILENAME;
        mLoggerFilePath   = CAFFEINE_LOG_FILENAME;
        mCustomIconsPath  = "Icons/";
    }
    else
    {
        auto appData = GetAppDataPath() / CAFFEINE_PROGRAM_NAME;
        fs::create_directory(appData);
    
        mSettingsFilePath = appData / CAFFEINE_SETTINGS_FILENAME;
        mLoggerFilePath   = appData / CAFFEINE_LOG_FILENAME;
        mCustomIconsPath  = appData / "Icons" / "";
    }

    mLogger->Open(mLoggerFilePath);
    Log("---- Log started ----");
}

CaffeineTray::~CaffeineTray()
{
    if (mSettingsChanged)
    {
        SaveSettings();
    }

    DisableCaffeine();

    Log("---- Log ended ----");
}

auto CaffeineTray::Init() -> bool
{
    Log("Initializing CaffeineTray...");

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

        mLightTheme    = IsLightTheme();
        mSessionLocked = IsSessionLocked();

        Log("System theme " + ToString((mLightTheme ? "(light)" : "(dark)")));
        Log("Session is " + ToString((mSessionLocked ? "locked" : "unlocked")));
    }

    // For hyperlinks in About dialog.
    {
        auto ccs   = INITCOMMONCONTROLSEX{ 0 };
        ccs.dwSize = sizeof(ccs);
        ccs.dwICC  = ICC_LINK_CLASS;
        InitCommonControlsEx(&ccs);
    }

    // Create NotifyIcon.
    {
        if (CreateNotifyIcon() != 0)
        {
            Log("Failed to create NotifyIcon");
            return false;
        }

        Log("Created NotifyIcon");
    }

    // Load custom icons.
    {
        LoadCustomIcon(0);
    }

    // Update icons, timer, power settings.
    {
        SetCaffeineMode(mSettings->Mode);
        UpdateAppIcon();
    }

    mInitialized = true;
    Log("Initialization finished");

    return true;
}

auto CaffeineTray::OnCreate() -> bool
{
    // Add session lock notification event.
    if (!WTSRegisterSessionNotification(mWindowHandle, NOTIFY_FOR_THIS_SESSION))
    {
        Log("Failed to register session notification event");
        Log("DisableOnLockScreen functionality will not work");
    }

    return true;
}

auto CaffeineTray::OnDestroy() -> void
{
    Log("Shutting down application");
    WTSUnRegisterSessionNotification(mWindowHandle);
}

auto CaffeineTray::OnCommand(WPARAM wParam, LPARAM lParam) -> bool
{
    switch (LOWORD(wParam))
    {
    case IDM_TOGGLE_CAFFEINE:
        ToggleCaffeineMode();
        return true;

    case IDM_DISABLE_CAFFEINE:
        SetCaffeineMode(CaffeineMode::Disabled);
        return true;

    case IDM_ENABLE_CAFFEINE:
        SetCaffeineMode(CaffeineMode::Enabled);
        return true;

    case IDM_ENABLE_AUTO:
        SetCaffeineMode(CaffeineMode::Auto);
        return true;

    case IDM_SETTINGS:
        ShowCaffeineSettings();
        return true;

    case IDM_ABOUT:
        ShowAboutDialog();
        return true;

    case IDM_EXIT:
        DestroyWindow(mWindowHandle);
        return true;
    }

    return false;
}

auto CaffeineTray::OnClick(WPARAM wParam, LPARAM lParam) -> bool
{
    Log("NotifyIcon Click");
    ToggleCaffeineMode();
    return true;
}

auto CaffeineTray::OnContextMenu(WPARAM wParam, LPARAM lParam) -> bool
{
    auto hMenu = HMENU{ 0 };
    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        hMenu = LoadMenuW(mInstanceHandle, MAKEINTRESOURCE(IDC_CAFFEINE_DISABLED_CONTEXTMENU));
        break;
    case CaffeineMode::Enabled:
        hMenu = LoadMenuW(mInstanceHandle, MAKEINTRESOURCE(IDC_CAFFEINE_ENABLED_CONTEXTMENU));
        break;
    case CaffeineMode::Auto:
        hMenu = LoadMenuW(mInstanceHandle, MAKEINTRESOURCE(IDC_CAFFEINE_AUTO_CONTEXTMENU));
        break;
    }

    if (hMenu)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu)
        {
            // our window must be foreground before calling TrackPopupMenu
            // or the menu will not disappear when the user clicks away
            SetForegroundWindow(mWindowHandle);

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

            TrackPopupMenuEx(hSubMenu, uFlags, LOWORD(wParam), HIWORD(wParam), mWindowHandle, NULL);
        }

        DestroyMenu(hMenu);
    }
    return true;
}

auto CaffeineTray::CustomDispatch(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
{
    switch (message)
    {

    case WM_WININICHANGE:
    {
        if (lParam)
        {
            auto str = reinterpret_cast<const wchar_t*>(lParam);
            auto sysparam = std::wstring_view(str);
            if (sysparam == L"ImmersiveColorSet")
            {
                mLightTheme = IsLightTheme();
                Log("System theme changed, new theme " + ToString((mLightTheme ? "(light)" : "(dark)")));
                UpdateIcon();
                UpdateAppIcon();
            }
        }

        return 0;
    }

    case WM_WTSSESSION_CHANGE:
    {
        switch (wParam)
        {
        case WTS_SESSION_LOCK:
            Log("Session locked");
            mSessionLocked = true;
            UpdateExecutionState();
            return 0;
        case WTS_SESSION_UNLOCK:
            Log("Session unlocked");
            mSessionLocked = false;
            UpdateExecutionState();
            return 0;
        }

        break;
    }

    case WM_DPICHANGED:
    {
        Log("Dpi changed");
        UpdateIcon();
        break;
    }

    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

auto CaffeineTray::EnableCaffeine () -> bool
{
    auto keepDisplayOn = false;
    auto disableOnLock = false;

    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        return false;

    case CaffeineMode::Enabled:
        keepDisplayOn = mSettings->Standard.KeepDisplayOn;
        disableOnLock = mSettings->Standard.DisableOnLockScreen;
        break;

    case CaffeineMode::Auto:
        keepDisplayOn = mSettings->Auto.KeepDisplayOn;
        disableOnLock = mSettings->Auto.DisableOnLockScreen;
        break;
    }

    if (mSessionLocked && keepDisplayOn)
    {
        keepDisplayOn = !disableOnLock;
    }

    auto result = mCaffeine.SystemRequired(keepDisplayOn);
    if (result)
    {
        Log("Caffeine Enabled");
        return true;
    }

    return false;
}

auto CaffeineTray::DisableCaffeine () -> bool
{
    if (mCaffeine.Continuous())
    {
        Log("Caffeine Disabled");
        return true;
    }

    return false;
}

auto CaffeineTray::UpdateExecutionState(bool activate) -> void
{
    auto guard = std::lock_guard(mCaffeineMutex);

    // NOTE: Auto mode deactivate caffeine by default.
    Log("Updating ExecutionState");

    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        DisableCaffeine();
        break;

    case CaffeineMode::Enabled:
        EnableCaffeine();
        break;

    case CaffeineMode::Auto:
        if (activate)
        {
            EnableCaffeine();
        }
        else
        {
            DisableCaffeine();
        }
        break;
    }

    UpdateIcon();
}

auto CaffeineTray::ToggleCaffeineMode() -> void
{
    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        mSettings->Mode = CaffeineMode::Enabled;
        break;
    case CaffeineMode::Enabled:
        mSettings->Mode = CaffeineMode::Auto;
        break;
    case CaffeineMode::Auto:
        mSettings->Mode = CaffeineMode::Disabled;
        break;
    }

    Log("Setting CaffeineMode to " + ToString(CaffeineModeToString(mSettings->Mode)));

    ResetTimer();
    UpdateExecutionState();

    mSettingsChanged = true;
}

auto CaffeineTray::SetCaffeineMode(CaffeineMode mode) -> void
{
    mSettings->Mode = mode;
    Log("Setting CaffeineMode to " + ToString(CaffeineModeToString(mSettings->Mode)));

    ResetTimer();
    UpdateExecutionState();

    mSettingsChanged = true;
}

auto CaffeineTray::UpdateIcon() -> bool
{
    auto icon = HICON{0};
    auto tip  = UINT{0};

    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
        icon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_DISABLED);
        tip = IDS_CAFFEINE_DISABLED;
        break;
    case CaffeineMode::Enabled:
        icon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_ENABLED);
        tip = IDS_CAFFEINE_ENABLED;
        break;
    case CaffeineMode::Auto:
        if (!mCaffeine.IsSystemRequired())
        {
            icon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE);
            tip = IDS_CAFFEINE_AUTO_INACTIVE;
        }
        else
        {
            icon = LoadIconHelper(IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE);
            tip = IDS_CAFFEINE_AUTO_ACTIVE;
        }
        break;
    }

    if (!UpdateNotifyIcon(icon, tip))
    {
        Log("Failed to update notification icon");
        return false;
    }

    Log("Updated notification icon");
    return true;
}

auto CaffeineTray::UpdateAppIcon() -> void
{
    auto icon = LoadIconHelper(IDI_CAFFEINE_APP);
    SendMessage(mWindowHandle, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
    SendMessage(mWindowHandle, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
}

auto CaffeineTray::LoadIconHelper(UINT icon) -> HICON
{
    const auto flags = UINT{ LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED };

    // Use user icon.
    auto userIcon = LoadCustomIcon(icon);
    if (userIcon)
    {
        return userIcon;
    }

    if (mSettings->UseNewIcons)
    {
        return LoadSquaredIcon(icon);
    }

    // Use default icons.
    auto id = UINT{ 32512 };
    if (mLightTheme)
    {
        switch (icon)
        {
        case IDI_CAFFEINE_APP:                  id = IDI_CAFFEINE_APP_DARK;                           break;
        case IDI_NOTIFY_CAFFEINE_DISABLED:      id = IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_DARK;      break;
        case IDI_NOTIFY_CAFFEINE_ENABLED:       id = IDI_NOTIFY_ORIGINAL_CAFFEINE_ENABLED_DARK;       break;
        case IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE: id = IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_DARK; break;
        case IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE:   id = IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_DARK;   break;
        }
    }
    else
    {
        switch (icon)
        {
        case IDI_CAFFEINE_APP:                  id = IDI_CAFFEINE_APP_LIGHT;                           break;
        case IDI_NOTIFY_CAFFEINE_DISABLED:      id = IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_LIGHT;      break;
        case IDI_NOTIFY_CAFFEINE_ENABLED:       id = IDI_NOTIFY_ORIGINAL_CAFFEINE_ENABLED_LIGHT;       break;
        case IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE: id = IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_LIGHT; break;
        case IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE:   id = IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_LIGHT;   break;
        }
    }
    
    return static_cast<HICON>(
        LoadImageW(mInstanceHandle, MAKEINTRESOURCEW(id), IMAGE_ICON, 0, 0, flags)
        );
}

auto CaffeineTray::LoadSquaredIcon(UINT icon) -> HICON
{
    const auto flags = UINT{ LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED };

    // Use default icons.
    auto id = UINT{ 32512 };
    if (mLightTheme)
    {
        switch (icon)
        {
        case IDI_CAFFEINE_APP:                  id = IDI_CAFFEINE_APP_DARK;                         break;
        case IDI_NOTIFY_CAFFEINE_DISABLED:      id = IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_DARK;      break;
        case IDI_NOTIFY_CAFFEINE_ENABLED:       id = IDI_NOTIFY_SQUARE_CAFFEINE_ENABLED_DARK;       break;
        case IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE: id = IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_INACTIVE_DARK; break;
        case IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE:   id = IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_ACTIVE_DARK;   break;
        }
    }
    else
    {
        switch (icon)
        {
        case IDI_CAFFEINE_APP:                  id = IDI_CAFFEINE_APP_LIGHT;                         break;
        case IDI_NOTIFY_CAFFEINE_DISABLED:      id = IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_LIGHT;      break;
        case IDI_NOTIFY_CAFFEINE_ENABLED:       id = IDI_NOTIFY_SQUARE_CAFFEINE_ENABLED_LIGHT;       break;
        case IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE: id = IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_INACTIVE_LIGHT; break;
        case IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE:   id = IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_ACTIVE_LIGHT;   break;
        }
    }
    
    return static_cast<HICON>(
        LoadImageW(mInstanceHandle, MAKEINTRESOURCEW(id), IMAGE_ICON, 0, 0, flags)
        );
}

auto CaffeineTray::LoadCustomIcon(UINT icon) -> HICON
{
    // Reload from file.
    if (icon == 0)
    {
        const auto flags = UINT{ LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED | LR_LOADFROMFILE };

        auto loadIco = [&](std::string fileName) -> HICON
        {
            auto path = mCustomIconsPath / fileName;
            if (fs::exists(path))
            {
                auto wpath = path.wstring();
                return static_cast<HICON>(LoadImageW(NULL, wpath.c_str(), IMAGE_ICON, 0, 0, flags));
            }

            return NULL;
        };

        mIconPack.caffeineDisabledDark  = loadIco("CaffeineDisabledDark.ico");
        mIconPack.caffeineDisabledLight = loadIco("CaffeineDisabledLight.ico");
        
        mIconPack.caffeineEnabledDark  = loadIco("CaffeineEnabledDark.ico");
        mIconPack.caffeineEnabledLight = loadIco("CaffeineEnabledLight.ico");
        
        mIconPack.caffeineAutoInactiveDark  = loadIco("CaffeineAutoInactiveDark.ico");
        mIconPack.caffeineAutoInactiveLight = loadIco("CaffeineAutoInactiveLight.ico");
        
        mIconPack.caffeineAutoActiveDark  = loadIco("CaffeineAutoActiveDark.ico");
        mIconPack.caffeineAutoActiveLight = loadIco("CaffeineAutoActiveLight.ico");
    }
    else
    {
        auto customIcon = HICON{NULL};
        if (mLightTheme)
        {
            switch (icon)
            {
            case IDI_NOTIFY_CAFFEINE_DISABLED:      customIcon = mIconPack.caffeineDisabledDark;     break;
            case IDI_NOTIFY_CAFFEINE_ENABLED:       customIcon = mIconPack.caffeineEnabledDark;      break;
            case IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE: customIcon = mIconPack.caffeineAutoInactiveDark; break;
            case IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE:   customIcon = mIconPack.caffeineAutoActiveDark;   break;
            }
        }
        else
        {
            switch (icon)
            {
            case IDI_NOTIFY_CAFFEINE_DISABLED:      customIcon = mIconPack.caffeineDisabledLight;     break;
            case IDI_NOTIFY_CAFFEINE_ENABLED:       customIcon = mIconPack.caffeineEnabledLight;      break;
            case IDI_NOTIFY_CAFFEINE_AUTO_INACTIVE: customIcon = mIconPack.caffeineAutoInactiveLight; break;
            case IDI_NOTIFY_CAFFEINE_AUTO_ACTIVE:   customIcon = mIconPack.caffeineAutoActiveLight;   break;
            }
        }

        return customIcon;
    }

    return NULL;
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

auto CaffeineTray::ResetTimer() -> bool
{
    mScannerTimer.Interval(std::chrono::milliseconds(mSettings->Auto.ScanInterval));

    switch (mSettings->Mode)
    {
    case CaffeineMode::Disabled:
    case CaffeineMode::Enabled:
        if (mScannerTimer.IsRunning())
        {
            Log("Killing timer");
            mScannerTimer.Pause();
        }
        break;

    case CaffeineMode::Auto:
        if (mScannerTimer.IsPaused() || mScannerTimer.IsStopped())
        {
            Log("Starting timer");
            mScannerTimer.Start();
        }
        break;
    }

    return true;
}

auto CaffeineTray::TimerUpdate() -> void
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
        if (!mCaffeine.IsSystemRequired())
        {
            if (foundProcess)
            {
                Log("Found process '" + ToString(mProcessScanner.GetLastFound()) + "'");
            }
            else
            {
                Log("Found window '" + ToString(mWindowScanner.GetLastFound()) + "'");
            }

            UpdateExecutionState(true);
        }
    }
    else
    {
        // Dectivate only if active.
        if (mCaffeine.IsSystemRequired())
        {
            Log("Process/window no longer exists");
            
            UpdateExecutionState(false);
        }
    }
}

auto CaffeineTray::Log(std::string message) -> void
{
    mLogger->Log(std::move(message));
}

auto CaffeineTray::ShowCaffeineSettings () -> bool
{
    SINGLE_INSTANCE_GUARD();
    
    auto caffeineSettings = CaffeineSettings(mSettings);
    if (caffeineSettings.Show(mWindowHandle))
    {
        const auto& newSettings = caffeineSettings.Result();

        mSettings->Standard = newSettings.Standard;
        mSettings->Auto     = newSettings.Auto;

        // Update with new settings.
        ResetTimer();
        UpdateExecutionState();

        mSettingsChanged = true;
    }

    return true;
}

auto CaffeineTray::ShowAboutDialog () -> bool
{
    SINGLE_INSTANCE_GUARD();
    
    auto aboutDlg = AboutDialog();
    aboutDlg.Show(mWindowHandle);

    return true;
}

} // namespace Caffeine
