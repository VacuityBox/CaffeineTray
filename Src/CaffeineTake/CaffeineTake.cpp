#include "CaffeineTake.hpp"

#include "Dialogs/AboutDialog.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "Resource.h"
#include "Utility.hpp"
#include "Version.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <commctrl.h>
#include <Psapi.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <WtsApi32.h>

using namespace std;

namespace CaffeineTake {

CaffeineTakeApp::CaffeineTakeApp(const AppInitInfo& info)
    : mSettings           (std::make_shared<Settings>())
    , mInstanceHandle     (info.InstanceHandle)
    , mLightTheme         (false)
    , mInitialized        (false)
    , mSessionLocked      (false)
    , mProcessScanner     (mSettings)
    , mWindowScanner      (mSettings)
    , mScannerTimer       (std::bind(&CaffeineTakeApp::TimerUpdate, this))
    , mCaffeine           ()
    , mNotifyIcon         (mni::NotifyIcon::Desc{
            .instance    = info.InstanceHandle,
            .windowTitle = L"CaffeineTray_InvisibleWindow",
            .className   = L"CaffeineTray_WndClass"
        }
    )
{
    mSettingsFilePath = info.SettingsPath;
    mCustomIconsPath  = info.DataDirectory / "Icons" / "";

    Log("---- Log started ----");
}

CaffeineTakeApp::~CaffeineTakeApp()
{
    DisableCaffeine();

    Log("---- Log ended ----");
}

auto CaffeineTakeApp::Init() -> bool
{
    Log("Initializing CaffeineTake...");

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
        if (FAILED(mNotifyIcon.Init()))
        {
            Log("Failed to create NotifyIcon");
            return false;
        }

        Log("Created NotifyIcon");

        mNotifyIcon.OnCreate            = std::bind(&CaffeineTakeApp::OnCreate, this);
        mNotifyIcon.OnDestroy           = std::bind(&CaffeineTakeApp::OnDestroy, this);
        mNotifyIcon.OnLmbClick          = std::bind(&CaffeineTakeApp::OnClick, this, std::placeholders::_1, std::placeholders::_2);
        mNotifyIcon.OnContextMenuOpen   = std::bind(&CaffeineTakeApp::OnContextMenuOpen, this);
        mNotifyIcon.OnContextMenuSelect = std::bind(&CaffeineTakeApp::OnContextMenuSelect, this, std::placeholders::_1);
        mNotifyIcon.OnThemeChange       = std::bind(&CaffeineTakeApp::OnThemeChange, this, std::placeholders::_1);
        mNotifyIcon.OnDpiChange         = std::bind(&CaffeineTakeApp::OnDpiChange, this, std::placeholders::_1);
        mNotifyIcon.OnSystemMessage     = std::bind(&CaffeineTakeApp::OnSystemMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

        mNotifyIcon.Show();
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

auto CaffeineTakeApp::MainLoop () -> int
{
    return mNotifyIcon.MainLoop();
}

auto CaffeineTakeApp::OnCreate() -> void
{
    // Add session lock notification event.
    if (!WTSRegisterSessionNotification(mNotifyIcon.Handle(), NOTIFY_FOR_THIS_SESSION))
    {
        Log("Failed to register session notification event");
        Log("DisableOnLockScreen functionality will not work");
    }
}

auto CaffeineTakeApp::OnDestroy() -> void
{
    Log("Shutting down application");
    WTSUnRegisterSessionNotification(mNotifyIcon.Handle());
}


auto CaffeineTakeApp::OnClick(int x, int y) -> void
{
    Log("NotifyIcon Click");
    ToggleCaffeineMode();
}

auto CaffeineTakeApp::OnContextMenuOpen() -> void
{
    auto hMenu = HMENU{0};
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

    mNotifyIcon.SetMenu(hMenu);
}

auto CaffeineTakeApp::OnContextMenuSelect(int selectedItem) -> void
{
    switch (selectedItem)
    {
    // TODO is this used?
    case IDM_TOGGLE_CAFFEINE:
        ToggleCaffeineMode();
        return;

    case IDM_DISABLE_CAFFEINE:
        SetCaffeineMode(CaffeineMode::Disabled);
        return;

    case IDM_ENABLE_CAFFEINE:
        SetCaffeineMode(CaffeineMode::Enabled);
        return;

    case IDM_ENABLE_AUTO:
        SetCaffeineMode(CaffeineMode::Auto);
        return;

    case IDM_SETTINGS:
        ShowCaffeineSettings();
        return;

    case IDM_ABOUT:
        ShowAboutDialog();
        return;

    case IDM_EXIT:
        mNotifyIcon.Quit();
        return;
    }
}

auto CaffeineTakeApp::OnThemeChange(mni::ThemeInfo ti) -> void
{
    mLightTheme = ti.IsLight();
    Log("System theme changed, new theme " + ToString((mLightTheme ? "(light)" : "(dark)")));
    UpdateIcon(mCaffeine.IsSystemRequired());
    UpdateAppIcon();
}

auto CaffeineTakeApp::OnDpiChange(int dpi) -> void
{
    UpdateIcon(mCaffeine.IsSystemRequired());
}

auto CaffeineTakeApp::OnSystemMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) -> bool
{
    switch (uMsg)
    {
    case WM_WTSSESSION_CHANGE:
        switch (wParam)
        {
        case WTS_SESSION_LOCK:
            Log("Session locked");
            mSessionLocked = true;
            UpdateExecutionState();
            return true;

        case WTS_SESSION_UNLOCK:
            Log("Session unlocked");
            mSessionLocked = false;
            UpdateExecutionState();
            return true;
        }

        break;
    }

    return false;
}

auto CaffeineTakeApp::EnableCaffeine () -> bool
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

auto CaffeineTakeApp::DisableCaffeine () -> bool
{
    if (mCaffeine.Continuous())
    {
        Log("Caffeine Disabled");
        return true;
    }

    return false;
}

auto CaffeineTakeApp::UpdateExecutionState(bool activate) -> void
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

    UpdateIcon(activate);
}

auto CaffeineTakeApp::ToggleCaffeineMode() -> void
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

    SaveSettings();
}

auto CaffeineTakeApp::SetCaffeineMode(CaffeineMode mode) -> void
{
    mSettings->Mode = mode;
    Log("Setting CaffeineMode to " + ToString(CaffeineModeToString(mSettings->Mode)));

    ResetTimer();
    UpdateExecutionState();

    SaveSettings();
}

auto CaffeineTakeApp::UpdateIcon(bool autoActive) -> bool
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
        //if (!mCaffeine.IsSystemRequired())
        if (!autoActive)
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

    if (FAILED(mNotifyIcon.SetIcon(icon)))
    {
        Log("Failed to update notification icon");
        return false;
    }

    WCHAR buffer[128];
    LoadStringW(mInstanceHandle, tip, buffer, ARRAYSIZE(buffer));

    if (FAILED(mNotifyIcon.SetTip(buffer)))
    {
        Log("Failed to update notification icon");
        return false;
    }

    Log("Updated notification icon");
    return true;
}

auto CaffeineTakeApp::UpdateAppIcon() -> void
{
    auto icon = LoadIconHelper(IDI_CAFFEINE_APP);
    SendMessage(mNotifyIcon.Handle(), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
    SendMessage(mNotifyIcon.Handle(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
}

auto CaffeineTakeApp::LoadIconHelper(UINT icon) -> HICON
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

auto CaffeineTakeApp::LoadSquaredIcon(UINT icon) -> HICON
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

auto CaffeineTakeApp::LoadCustomIcon(UINT icon) -> HICON
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

auto CaffeineTakeApp::LoadSettings() -> bool
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
    
    try
    {
        *mSettings = json.get<Settings>();
    }
    catch (nlohmann::json::exception&)
    {
        Log("Failed to deserialize settings");
        Log("Using default values");
        *mSettings = Settings();
        return true;
    }

    //Log() << json.dump(4).c_str() << std::endl;
    Log("Loaded Settings '" + mSettingsFilePath.string() + "'");

    return true;
}

auto CaffeineTakeApp::SaveSettings() -> bool
{
    // Open Settings file.
    auto file = std::ofstream(mSettingsFilePath);
    if (!file)
    {
        Log("Can't open Settings file '" + mSettingsFilePath.string() + "' for writing");
        return false;
    }

    // Serialize.
    auto json = nlohmann::json(*mSettings);
    file << std::setw(4) << json;

    Log("Saved Settings '" + mSettingsFilePath.string() + "'");
    return true;
}

auto CaffeineTakeApp::ResetTimer() -> bool
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

auto CaffeineTakeApp::TimerUpdate() -> void
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

auto CaffeineTakeApp::Log(std::string message) -> void
{
    spdlog::info(message);
}

auto CaffeineTakeApp::ShowCaffeineSettings () -> bool
{
    SINGLE_INSTANCE_GUARD();
    
    auto caffeineSettings = CaffeineSettings(mSettings);
    // HACK
    //if (caffeineSettings.Show(mWindowHandle))
    if (caffeineSettings.Show(NULL))
    {
        const auto& newSettings = caffeineSettings.Result();

        mSettings->Standard = newSettings.Standard;
        mSettings->Auto     = newSettings.Auto;

        // Update with new settings.
        ResetTimer();
        UpdateExecutionState();

        SaveSettings();
    }

    return true;
}

auto CaffeineTakeApp::ShowAboutDialog () -> bool
{
    SINGLE_INSTANCE_GUARD();
    
    auto aboutDlg = AboutDialog();
    // HACK
    //aboutDlg.Show(mWindowHandle);
    aboutDlg.Show(NULL);

    return true;
}

} // namespace CaffeineTake
