// CaffeineTake - Keep your computer awake.
// 
// Copyright (c) 2020-2021 VacuityBox
// Copyright (c) 2022      serverfailure71
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CaffeineApp.hpp"

#include "Dialogs/AboutDialog.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "Logger.hpp"
#include "Resource.hpp"
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

CaffeineApp::CaffeineApp (const AppInitInfo& info)
    : mSettings           (std::make_shared<Settings>())
    , mLang               (std::make_shared<Lang>())
    , mSettingsFilePath   (info.SettingsPath)
    , mCustomIconsPath    (info.DataDirectory / "Icons" / "")
    , mLangDirectory      (info.DataDirectory / "Lang" / "")
    , mInstanceHandle     (info.InstanceHandle)
    , mInitialized        (false)
    , mSessionState       (SessionState::Unlocked)
    , mNotifyIcon         (mni::NotifyIcon::Desc{
            .instance    = info.InstanceHandle,
            .windowTitle = L"CaffeineTray_InvisibleWindow",
            .className   = L"CaffeineTray_WndClass"
        }
    )
    , mThemeInfo           (mni::ThemeInfo::Detect())
    , mIcons               (info.InstanceHandle)
    , mCaffeineState       (CaffeineState::Inactive)
    , mCaffeineMode        (CaffeineMode::Disabled)
    , mKeepDisplayOn       (false)
    , mAppSO               (this)
    , mDisabledMode        (mAppSO)
    , mEnabledMode         (mAppSO)
    , mAutoMode            (mAppSO)
    , mTimerMode           (mAppSO)
    , mDpi                 (96)
    , mCurrentMode         (nullptr)
{
}

CaffeineApp::~CaffeineApp()
{
    DisableCaffeine();
}

auto CaffeineApp::Init() -> bool
{
    LOG_INFO("Initializing CaffeineTake...");

    // Load Settings.
    {
        // Create default settings file if not exists.
        if (!fs::exists(mSettingsFilePath))
        {
            LOG_WARNING("Settings file not found, creating default one");
            SaveSettings();
        }
        else
        {
            if (!LoadSettings())
            {
                return false;
            }
        }
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
            LOG_ERROR("Failed to create NotifyIcon");
            return false;
        }

        LOG_INFO("Created NotifyIcon");

        // Register callbacks.
        mNotifyIcon.OnCreate            = std::bind(&CaffeineApp::OnCreate, this);
        mNotifyIcon.OnDestroy           = std::bind(&CaffeineApp::OnDestroy, this);
        mNotifyIcon.OnLmbClick          = std::bind(&CaffeineApp::OnClick, this, std::placeholders::_1, std::placeholders::_2);
        mNotifyIcon.OnContextMenuOpen   = std::bind(&CaffeineApp::OnContextMenuOpen, this);
        mNotifyIcon.OnContextMenuSelect = std::bind(&CaffeineApp::OnContextMenuSelect, this, std::placeholders::_1);
        mNotifyIcon.OnThemeChange       = std::bind(&CaffeineApp::OnThemeChange, this, std::placeholders::_1);
        mNotifyIcon.OnDpiChange         = std::bind(&CaffeineApp::OnDpiChange, this, std::placeholders::_1);
        mNotifyIcon.OnCustomMessage     = std::bind(&CaffeineApp::OnCustomMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        mNotifyIcon.OnSystemMessage     = std::bind(&CaffeineApp::OnSystemMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

        mNotifyIcon.Show();
    }

    // Get theme/dpi.
    {
        mDpi = GetDpi(mNotifyIcon.Handle());
        mThemeInfo = mni::ThemeInfo::Detect();
        mSessionState = IsSessionLocked();

        LOG_INFO("System dpi: {}", mDpi);
        LOG_INFO("System theme: {}", static_cast<int>(mThemeInfo.GetTheme()));
        LOG_INFO("Session state: {}", mSessionState == SessionState::Unlocked ? "Unlocked" : "Locked");
    }

    // Load icons.
    {
        const auto w = (16 * mDpi) / 96;
        const auto h = (16 * mDpi) / 96;

        mIcons.Load(mSettings->General.IconPack, mThemeInfo.IsDark() ? CaffeineIcons::Theme::Light : CaffeineIcons::Theme::Dark, w, h);
    }

    // Load language.
    {
        LoadLang();
    }

    // Update mode, icons, execution state, tip.
    {
        if (!LoadMode())
        {
            LOG_INFO("Writing default mode to registry");
            SaveMode();
        }

        SetCaffeineMode(mCaffeineMode);
        UpdateAppIcon();
    }

    mInitialized = true;
    LOG_INFO("Initialization finished");

    return true;
}

auto CaffeineApp::MainLoop () -> int
{
    return mNotifyIcon.MainLoop();
}

auto CaffeineApp::OnCreate() -> void
{
    // Add session lock notification event.
    if (!WTSRegisterSessionNotification(mNotifyIcon.Handle(), NOTIFY_FOR_THIS_SESSION))
    {
        LOG_ERROR("Failed to register session notification event");
        LOG_INFO("DisableOnLockScreen functionality will not work");
    }
}

auto CaffeineApp::OnDestroy() -> void
{
    LOG_INFO("Shutting down application");
    // TODO set caffeine mdoe to disabled here instead destructor?
    WTSUnRegisterSessionNotification(mNotifyIcon.Handle());
}


auto CaffeineApp::OnClick(int x, int y) -> void
{
    LOG_TRACE("NotifyIcon::OnClick");
    ToggleCaffeineMode();
}

auto CaffeineApp::OnContextMenuOpen() -> void
{
    LOG_TRACE("NotifyIcon::OnContextMenuOpen");

    auto hPopupMenu = CreatePopupMenu();
    if (!hPopupMenu)
    {
        LOG_ERROR("Failed to create popup menu, error: {}", GetLastError());
        return;
    }

    auto hMenu = CreateMenu();
    if (!hMenu)
    {
        LOG_ERROR("Failed to create menu, error: {}", GetLastError());
        return;
    }

    switch (mCaffeineMode)
    {
    case CaffeineMode::Disabled:
        AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_CAFFEINE, mLang->ContextMenu_EnableCaffeine.c_str());
        AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_AUTO, mLang->ContextMenu_EnableAuto.c_str());
        AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_TIMER, mLang->ContextMenu_EnableTimer.c_str());
        break;
    case CaffeineMode::Enabled:
        AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_AUTO, mLang->ContextMenu_EnableAuto.c_str());
        AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_TIMER, mLang->ContextMenu_EnableTimer.c_str());
        AppendMenuW(hMenu, MF_STRING, IDM_DISABLE_CAFFEINE, mLang->ContextMenu_DisableCaffeine.c_str());
        break;
    case CaffeineMode::Auto:
        AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_CAFFEINE, mLang->ContextMenu_EnableCaffeine.c_str());
        AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_TIMER, mLang->ContextMenu_EnableTimer.c_str());
        AppendMenuW(hMenu, MF_STRING, IDM_DISABLE_CAFFEINE, mLang->ContextMenu_DisableCaffeine.c_str());
        break;
    case CaffeineMode::Timer:
        AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_CAFFEINE, mLang->ContextMenu_EnableCaffeine.c_str());
        AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_AUTO, mLang->ContextMenu_EnableAuto.c_str());
        AppendMenuW(hMenu, MF_STRING, IDM_DISABLE_CAFFEINE, mLang->ContextMenu_DisableCaffeine.c_str());
        break;
    }

    AppendMenuW(hMenu, MF_SEPARATOR, NULL, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, mLang->ContextMenu_Settings.c_str());
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, mLang->ContextMenu_Exit.c_str());

    AppendMenuW(hPopupMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenu), NULL);

    const auto hr = mNotifyIcon.SetMenu(hPopupMenu);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to set menu, error: {}", hr);
    }
}

auto CaffeineApp::OnContextMenuSelect(int selectedItem) -> void
{
    LOG_TRACE("NotifyIcon::OnContextMenuSelect(selectedItem={})", selectedItem);

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

    case IDM_ENABLE_TIMER:
        SetCaffeineMode(CaffeineMode::Timer);
        return;

    case IDM_SETTINGS:
        ShowSettingsDialog();
        return;

    case IDM_ABOUT:
        ShowAboutDialog();
        return;

    case IDM_EXIT:
        mNotifyIcon.Quit();
        return;
    }
}

auto CaffeineApp::OnThemeChange(mni::ThemeInfo ti) -> void
{
    LOG_INFO("System theme changed, new theme: {}", static_cast<int>(ti.GetTheme()));

    mThemeInfo = ti;

    const auto w = (16 * mDpi) / 96;
    const auto h = (16 * mDpi) / 96;

    // Load proper icons.
    // TODO pick right icons for high contrast
    mIcons.Load(mSettings->General.IconPack, mThemeInfo.IsDark() ? CaffeineIcons::Theme::Light : CaffeineIcons::Theme::Dark, w, h);

    UpdateIcon();
    UpdateAppIcon();
}

auto CaffeineApp::OnDpiChange(int dpi) -> void
{
    LOG_INFO("System dpi changed, new dpi: {}", dpi);

    mDpi = dpi;

    const auto w = (16 * dpi) / 96;
    const auto h = (16 * dpi) / 96;

    // TODO pick right icons for high contrast
    // it can be specific icon override
    mIcons.Load(mSettings->General.IconPack, mThemeInfo.IsDark() ? CaffeineIcons::Theme::Light : CaffeineIcons::Theme::Dark, w, h);

    UpdateIcon();
}

auto CaffeineApp::OnCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) -> void
{
    LOG_TRACE("NotifyIcon::OnCustomMessage(uMsg={})", uMsg);
    switch (uMsg)
    {
    case WM_CAFFEINE_TAKE_UPDATE_EXECUTION_STATE:
        if (static_cast<bool>(wParam))
        {
            UpdateExecutionState(CaffeineState::Active);
        }
        else
        {
            UpdateExecutionState(CaffeineState::Inactive);
        }
        break;
    }
}

auto CaffeineApp::OnSystemMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) -> bool
{
    LOG_TRACE("NotifyIcon::OnSystemMessage(uMsg={})", uMsg);
    switch (uMsg)
    {
    case WM_WTSSESSION_CHANGE:
        switch (wParam)
        {
        case WTS_SESSION_LOCK:
            LOG_INFO("Session lock event");
            mSessionState = SessionState::Locked;
            RefreshExecutionState();
            return true;

        case WTS_SESSION_UNLOCK:
            LOG_INFO("Session unlock event");
            mSessionState = SessionState::Unlocked;
            RefreshExecutionState();
            return true;
        }

        break;
    }

    return false;
}

auto CaffeineApp::EnableCaffeine () -> bool
{
    LOG_TRACE("EnableCaffeine()");
    mNotifyIcon.SendCustomMessage(WM_CAFFEINE_TAKE_UPDATE_EXECUTION_STATE, static_cast<WPARAM>(true), NULL);
    return true;
}

auto CaffeineApp::DisableCaffeine () -> bool
{
    LOG_TRACE("DisableCaffeine()");
    mNotifyIcon.SendCustomMessage(WM_CAFFEINE_TAKE_UPDATE_EXECUTION_STATE, static_cast<WPARAM>(false), NULL);
    return true;
}

auto CaffeineApp::ToggleCaffeineMode() -> void
{
    LOG_TRACE("ToggleCaffeineMode()");
    
    auto mode = CaffeineMode::Disabled;
    switch (mCaffeineMode)
    {
    case CaffeineMode::Disabled:
        mode = CaffeineMode::Enabled;
        break;
    case CaffeineMode::Enabled:
        mode = CaffeineMode::Auto;
        break;
    case CaffeineMode::Auto:
        mode = CaffeineMode::Timer;
        break;
    case CaffeineMode::Timer:
        mode = CaffeineMode::Disabled;
        break;
    }

    SetCaffeineMode(mode);
}

auto CaffeineApp::SetCaffeineMode(CaffeineMode mode) -> void
{
    LOG_INFO(L"Setting CaffeineMode to {}", CaffeineModeToString(mode));

    // Stop current mode.
    StopMode();

    // Start new one.
    mCaffeineMode = mode;
    switch (mCaffeineMode)
    {
    case CaffeineMode::Disabled: mCurrentMode = &mDisabledMode; break;
    case CaffeineMode::Enabled:  mCurrentMode = &mEnabledMode;  break;
    case CaffeineMode::Auto:     mCurrentMode = &mAutoMode;     break;
    case CaffeineMode::Timer:    mCurrentMode = &mTimerMode;    break;
    }

    StartMode();

    UpdateIcon();
    UpdateTip();

    SaveMode();
}

auto CaffeineApp::StartMode () -> void
{
    if (mCurrentMode)
    {
        mCurrentMode->Start();
    }
}

auto CaffeineApp::StopMode () -> void
{
    if (mCurrentMode)
    {
        mCurrentMode->Stop();
    }
}

auto CaffeineApp::LoadMode () -> bool
{
    auto subKey   = std::format(L"Software\\{}", CAFFEINE_TAKE_PROGRAM_NAME);
    auto data     = DWORD{0};
    auto dataSize = DWORD{sizeof(data)};
    auto status   = ::RegGetValueW(
        HKEY_CURRENT_USER,
        subKey.c_str(),
        L"CaffeineMode",
        RRF_RT_REG_DWORD,
        NULL,
        &data,
        &dataSize
    );

    auto mode   = CaffeineMode::Disabled;
    auto result = true;

    if (status != ERROR_SUCCESS)
    {
        LOG_ERROR("Failed to load CaffeineMode from registry");
        LOG_INFO("Using default Disabled mode");
        result = false;
    }
    else
    {
        LOG_INFO("Loaded CaffeineMode from registry");
        mode = static_cast<CaffeineMode>(data);
    }

    mCaffeineMode = mode;
    
    return result;
}

auto CaffeineApp::SaveMode () -> bool
{
    auto subKey   = std::format(L"Software\\{}", CAFFEINE_TAKE_PROGRAM_NAME);
    auto data     = static_cast<DWORD>(mCaffeineMode);
    auto dataSize = DWORD{sizeof(data)};
    auto status   = ::RegSetKeyValueW(
        HKEY_CURRENT_USER,
        subKey.c_str(),
        L"CaffeineMode",
        REG_DWORD,
        reinterpret_cast<LPCWSTR>(&data),
        dataSize
    );

    auto result = true;

    if (status != ERROR_SUCCESS)
    {
        LOG_ERROR("Failed to save CaffeineMode to registry");
        result = false;
    }
    else
    {
        LOG_INFO("Saved CaffeineMode to registry");
    }
    
    return result;
}

auto CaffeineApp::UpdateExecutionState(CaffeineState state) -> void
{
    auto keepDisplayOn = false;
    auto disableOnLock = false;

    switch (mCaffeineMode)
    {
    case CaffeineMode::Disabled:
        break;

    case CaffeineMode::Enabled:
        keepDisplayOn = mSettings->Standard.KeepDisplayOn;
        disableOnLock = mSettings->Standard.DisableOnLockScreen;
        break;

    case CaffeineMode::Auto:
        keepDisplayOn = mSettings->Auto.KeepDisplayOn;
        disableOnLock = mSettings->Auto.DisableOnLockScreen;
        break;

    case CaffeineMode::Timer:
        keepDisplayOn = mSettings->Timer.KeepDisplayOn;
        disableOnLock = mSettings->Timer.DisableOnLockScreen;
        break;
    }

    if (mCaffeineMode != CaffeineMode::Disabled)
    {
        if (mSessionState == SessionState::Locked && keepDisplayOn)
        {
            keepDisplayOn = !disableOnLock;
        }

        if (mCaffeineState == state && mKeepDisplayOn == keepDisplayOn)
        {
            LOG_DEBUG("No need to update execution state, continuing");
            return;
        }
    }
    else
    {
        if (mCaffeineState == state)
        {
            LOG_DEBUG("No need to update execution state, continuing");
            return;
        }
    }

    // Update Execution State.
    mCaffeineState = state;
    mKeepDisplayOn = keepDisplayOn;

    auto flags = EXECUTION_STATE{ES_CONTINUOUS};
    if (mCaffeineState == CaffeineState::Active)
    {
        flags |= ES_SYSTEM_REQUIRED;

        if (keepDisplayOn)
        {
            flags |= ES_DISPLAY_REQUIRED;
        }
    }

    if (!SetThreadExecutionState(flags))
    {
        LOG_ERROR("Failed to update execution state");
        return;
    }

    LOG_INFO("Updated execution state");

    UpdateIcon();
    UpdateTip();
}

auto CaffeineApp::RefreshExecutionState () -> void
{
    UpdateExecutionState(mCaffeineState);
}

auto CaffeineApp::UpdateIcon() -> bool
{
    auto icon = HICON{0};

    switch (mCaffeineMode)
    {
    case CaffeineMode::Disabled:
        icon = mIcons.CaffeineDisabled;
        break;
    case CaffeineMode::Enabled:
        icon = mIcons.CaffeineEnabled;
        break;
    case CaffeineMode::Auto:
        if (mCaffeineState == CaffeineState::Inactive)
        {
            icon = mIcons.CaffeineAutoInactive;
        }
        else
        {
            icon = mIcons.CaffeineAutoActive;
        }
        break;
    case CaffeineMode::Timer:
        if (mCaffeineState == CaffeineState::Inactive)
        {
            icon = mIcons.CaffeineTimerInactive;
        }
        else
        {
            icon = mIcons.CaffeineTimerActive;
        }
        break;
    }

    // No need to update.
    if (mNotifyIcon.GetIcon() == icon)
    {
        return false;
    }

    if (FAILED(mNotifyIcon.SetIcon(icon)))
    {
        LOG_ERROR("Failed to update notifyicon icon");
        return false;
    }

    LOG_INFO("Updated notifyicon icon");
    return true;
}

auto CaffeineApp::UpdateTip() -> bool
{
    auto tip  = UINT{0};

    switch (mCaffeineMode)
    {
    case CaffeineMode::Disabled:
        tip = IDS_CAFFEINE_DISABLED;
        break;
    case CaffeineMode::Enabled:
        tip = IDS_CAFFEINE_ENABLED;
        break;
    case CaffeineMode::Auto:
        if (mCaffeineState == CaffeineState::Inactive)
        {
            tip = IDS_CAFFEINE_AUTO_INACTIVE;
        }
        else
        {
            tip = IDS_CAFFEINE_AUTO_ACTIVE;
        }
        break;
    case CaffeineMode::Timer:
        if (mCaffeineState == CaffeineState::Inactive)
        {
            tip = IDS_CAFFEINE_TIMER_INACTIVE;
        }
        else
        {
            tip = IDS_CAFFEINE_TIMER_ACTIVE;
        }
        break;
    }

    auto buffer = std::array<wchar_t, 128>();
    buffer.fill(L'\0');
    
    LoadStringW(mInstanceHandle, tip, buffer.data(), buffer.size());

    // No need to update.
    if (mNotifyIcon.GetTip() == buffer.data())
    {
        return false;
    }

    if (FAILED(mNotifyIcon.SetTip(buffer.data())))
    {
        LOG_ERROR("Failed to update notifyicon tip");
        return false;
    }

    LOG_INFO("Updated notifyicon tip");
    return true;
}

auto CaffeineApp::UpdateAppIcon() -> void
{
    // TODO sometimes icon in taskmanger is invalid
    // TODO is this function needed, maybe making a proper icon to look good on both themes, white icon with black outline or something
    auto icon = [&](){
        const auto w = (16 * mDpi) / 96;
        const auto h = (16 * mDpi) / 96;

        const auto flags = UINT{ LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED };
        if (mThemeInfo.IsLight())
        {
            return static_cast<HICON>(
                LoadImageW(mInstanceHandle, MAKEINTRESOURCEW(IDI_CAFFEINE_APP_DARK), IMAGE_ICON, w, h, flags)
            );
        }
        else
        {
            return static_cast<HICON>(
                LoadImageW(mInstanceHandle, MAKEINTRESOURCEW(IDI_CAFFEINE_APP_LIGHT), IMAGE_ICON, w, h, flags)
            );
        }
    }();


    SendMessage(mNotifyIcon.Handle(), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
    SendMessage(mNotifyIcon.Handle(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
}

auto CaffeineApp::LoadSettings() -> bool
{
    // TODO return default settings on error always?
    // NOTE: Settings should be in UTF-8
    // Read Settings file.
    auto file = std::ifstream(mSettingsFilePath);
    if (!file)
    {
        LOG_ERROR("Can't open Settings file '{}' for reading", mSettingsFilePath.string());
        return false;
    }

    // Deserialize.
    auto json = nlohmann::json::parse(file, nullptr, false, true);
    if (json.is_discarded())
    {
        LOG_ERROR("Failed to parse json of settings file");
        return false;
    }
    
    try
    {
        *mSettings = json.get<Settings>();
    }
    catch (nlohmann::json::exception&)
    {
        LOG_ERROR("Failed to deserialize settings");
        LOG_WARNING("Using default values");

        mSettings.reset();
        mSettings = std::make_shared<Settings>();

        return true;
    }

    LOG_DEBUG("{}", json.dump(4));
    LOG_INFO("Loaded Settings '{}'", mSettingsFilePath.string());

    return true;
}

auto CaffeineApp::SaveSettings() -> bool
{
    // Open Settings file.
    auto file = std::ofstream(mSettingsFilePath);
    if (!file)
    {
        LOG_ERROR("Can't open Settings file '{}' for writing", mSettingsFilePath.string());
        return false;
    }

    // Serialize.
    auto json = nlohmann::json(*mSettings);
    file << std::setw(4) << json;

    LOG_INFO("Saved Settings '{}'", mSettingsFilePath.string());
    return true;
}

auto CaffeineApp::LoadLang () -> bool
{
    // NOTE: Language file should be in UTF-8
    // Read Lang file.
    const auto langPath = mLangDirectory / (mSettings->General.LangId + L".json");
    auto file = std::ifstream(langPath);
    if (!file)
    {
        LOG_ERROR("Can't open language file '{}' for reading", langPath.string());
        return LoadDefaultLang();
    }

    // Deserialize.
    auto json = nlohmann::json::parse(file, nullptr, false, true);
    if (json.is_discarded())
    {
        LOG_ERROR("Failed to parse json of lang file");
        return LoadDefaultLang();
    }
    
    try
    {
        *mLang = json.get<Lang>();
    }
    catch (nlohmann::json::exception&)
    {
        LOG_ERROR("Failed to deserialize language file");
        return LoadDefaultLang();
    }

    LOG_DEBUG("{}", json.dump(4));

    mLang->LangId = mSettings->General.LangId;
    // TODO get name from langid
    //mLang->LangName = 

    LOG_INFO(L"Loaded language {} ({}), file: '{}'", mLang->LangName, mLang->LangId, langPath.wstring());

    return true;
}

auto CaffeineApp::LoadDefaultLang () -> bool
{
    LOG_INFO("Using default language (en)");
        
    mLang.reset();
    mLang = std::make_shared<Lang>();

    return true;
}

auto CaffeineApp::ShowSettingsDialog () -> bool
{
    SINGLE_INSTANCE_GUARD();
    
    auto caffeineSettings = CaffeineSettings(mSettings);
    if (caffeineSettings.Show(mNotifyIcon.Handle()))
    {
        const auto& newSettings = caffeineSettings.Result();

        mSettings->Standard = newSettings.Standard;
        mSettings->Auto     = newSettings.Auto;

        // TODO in future settings change might change auto mode refresh interval, so update timer settings        

        // Settings change don't trigger caffeine state to change,
        // but display settings might change so we need to update.
        RefreshExecutionState();

        SaveSettings();
    }

    return true;
}

auto CaffeineApp::ShowAboutDialog () -> bool
{
    SINGLE_INSTANCE_GUARD();
    
    auto aboutDlg = AboutDialog();
    aboutDlg.Show(mNotifyIcon.Handle());

    return true;
}

} // namespace CaffeineTake
