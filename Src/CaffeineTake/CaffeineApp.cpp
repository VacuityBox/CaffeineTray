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
#include "JumpList.hpp"
#include "Lang.hpp"
#include "Logger.hpp"
#include "Resource.hpp"
#include "Settings.hpp"
#include "Utility.hpp"
#include "Version.hpp"

#include <filesystem>
#include <fstream>

#include <commctrl.h>
#include <Psapi.h>
#include <shellapi.h>
#include <ShlObj.h>

#if defined(FEATURE_CAFFEINETAKE_LOCKSCREEN_DETECTION)
#   include <WtsApi32.h>
#endif

using namespace std;

namespace CaffeineTake {

CaffeineApp::CaffeineApp (const AppInitInfo& info)
    : mSettings           (std::make_shared<Settings>())
    , mLang               (std::make_shared<Lang>())
    , mExecutablePath     (info.ExecutablePath)
    , mSettingsFilePath   (info.SettingsPath)
    , mCustomIconsPath    (info.DataDirectory / "Icons" / "")
    , mLangDirectory      (info.DataDirectory / "Lang" / "")
    , mInstanceHandle     (info.InstanceHandle)
    , mInitialized        (false)
    , mShuttingDown       (false)
    , mSessionState       (SessionState::Unlocked)
    , mNotifyIcon         ()
    , mThemeInfo          (mni::ThemeInfo::Detect())
    , mIcons              (info.InstanceHandle)
    , mCaffeineState      (CaffeineState::Inactive)
    , mCaffeineMode       (CaffeineMode::Disabled)
    , mKeepScreenOn       (false)
    , mAppSO              (this)
    , mDisabledMode       (mAppSO)
    , mEnabledMode        (mAppSO)
    , mAutoMode           (mAppSO)
    , mTimerMode          (mAppSO)
    , mDpi                (96)
    , mCurrentMode        (nullptr)
{
}

CaffeineApp::~CaffeineApp()
{
    mShuttingDown = true;
    SetCaffeineMode(CaffeineMode::Disabled);
    CoUninitialize();
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
            LoadSettings();
        }
    }

    // For hyperlinks in About dialog.
    {
        auto ccs   = INITCOMMONCONTROLSEX{ 0 };
        ccs.dwSize = sizeof(ccs);
        ccs.dwICC  = ICC_LINK_CLASS;
        InitCommonControlsEx(&ccs);
    }

    // For Jump Lists.
    {
        auto hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to CoInitializeEx(), hr: {}", hr);
            LOG_WARNING("Jump List functionality will not work");
        }
    }

    // Create NotifyIcon.
    {
        auto desc = mni::NotifyIcon::Desc{
            .instance    = mInstanceHandle,
            .windowTitle = L"CaffeineTake_InvisibleWindow",
            .className   = L"CaffeineTake_WndClass",
        };

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

        if (FAILED(mNotifyIcon.Init(desc)))
        {
            LOG_ERROR("Failed to create NotifyIcon");
            return false;
        }

        LOG_INFO("Created NotifyIcon");
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
#if defined(FEATURE_CAFFEINETAKE_LOCKSCREEN_DETECTION)
    // Add session lock notification event.
    if (!WTSRegisterSessionNotification(mNotifyIcon.Handle(), NOTIFY_FOR_THIS_SESSION))
    {
        LOG_ERROR("Failed to register session notification event");
        LOG_INFO("DisableOnLockScreen functionality will not work");
    }
#endif
}

auto CaffeineApp::OnDestroy() -> void
{
    LOG_INFO("Shutting down application");
#if defined(FEATURE_CAFFEINETAKE_LOCKSCREEN_DETECTION)
    WTSUnRegisterSessionNotification(mNotifyIcon.Handle());
#endif
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
        if (IsModeAvailable(CaffeineMode::Enabled)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_CAFFEINE, mLang->ContextMenu_EnableCaffeine.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Auto)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_AUTO, mLang->ContextMenu_EnableAuto.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Timer)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_TIMER, mLang->ContextMenu_EnableTimer.c_str());
        }
        break;
    case CaffeineMode::Enabled:
        if (IsModeAvailable(CaffeineMode::Auto)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_AUTO, mLang->ContextMenu_EnableAuto.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Timer)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_TIMER, mLang->ContextMenu_EnableTimer.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Disabled)) {
            AppendMenuW(hMenu, MF_STRING, IDM_DISABLE_CAFFEINE, mLang->ContextMenu_DisableCaffeine.c_str());
        }
        break;
    case CaffeineMode::Auto:
        if (IsModeAvailable(CaffeineMode::Enabled)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_CAFFEINE, mLang->ContextMenu_EnableCaffeine.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Timer)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_TIMER, mLang->ContextMenu_EnableTimer.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Disabled)) {
            AppendMenuW(hMenu, MF_STRING, IDM_DISABLE_CAFFEINE, mLang->ContextMenu_DisableCaffeine.c_str());
        }
        break;
    case CaffeineMode::Timer:
        if (IsModeAvailable(CaffeineMode::Enabled)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_CAFFEINE, mLang->ContextMenu_EnableCaffeine.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Auto)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_AUTO, mLang->ContextMenu_EnableAuto.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Disabled)) {
            AppendMenuW(hMenu, MF_STRING, IDM_DISABLE_CAFFEINE, mLang->ContextMenu_DisableCaffeine.c_str());
        }
        break;
    }

    AppendMenuW(hMenu, MF_SEPARATOR, NULL, NULL);

#if defined(FEATURE_CAFFEINETAKE_SETTINGS_DIALOG)
    AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, mLang->ContextMenu_Settings.c_str());
#else
    AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, mLang->ContextMenu_About.c_str());
#endif

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
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE)
        SetCaffeineMode(CaffeineMode::Auto);
#endif
        return;

    case IDM_ENABLE_TIMER:
#if defined(FEATURE_CAFFEINETAKE_TIMER_MODE)
        SetCaffeineMode(CaffeineMode::Timer);
#endif
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
        if (IsModeAvailable(CaffeineMode::Auto)) {
            mode = CaffeineMode::Auto;
        }
        else if (IsModeAvailable(CaffeineMode::Timer)) {
            mode = CaffeineMode::Timer;
        }
        else {
            mode = CaffeineMode::Disabled;
        }
        break;

    case CaffeineMode::Auto:
        if (IsModeAvailable(CaffeineMode::Timer)) {
            mode = CaffeineMode::Timer;
        }
        else {
            mode = CaffeineMode::Disabled;
        }
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

    auto nextMode = static_cast<Mode*>(nullptr);
    switch (mode)
    {
    case CaffeineMode::Disabled:
        if (IsModeAvailable(CaffeineMode::Disabled)) {
            nextMode = &mDisabledMode;
        }
        else {
            LOG_ERROR("Mode 'Disabled' is not available");
            return;
        }
        break;

    case CaffeineMode::Enabled:
        if (IsModeAvailable(CaffeineMode::Enabled)) {
            nextMode = &mEnabledMode;
        }
        else {
            LOG_ERROR("Mode 'Enabled' is not available");
            return;
        }
        break;

    case CaffeineMode::Auto:
        if (IsModeAvailable(CaffeineMode::Auto)) {
            nextMode = &mAutoMode;
        }
        else {
            LOG_ERROR("Mode 'Auto' is not available");
            return;
        }
        break;

    case CaffeineMode::Timer:
        if (IsModeAvailable(CaffeineMode::Timer)) {
            nextMode = &mTimerMode;
        }
        else {
            LOG_ERROR("Mode 'Timer' is not available");
            return;
        }
        break;
    }

    // Stop current mode.
    StopMode();

    // Start new one.
    mCaffeineMode = mode;
    mCurrentMode = nextMode;

    StartMode();

    UpdateIcon();
    UpdateTip();
    UpdateJumpList();

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

    // TODO check availability
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
    auto keepScreenOn      = false;
    auto whenSessionLocked = false;

    switch (mCaffeineMode)
    {
    case CaffeineMode::Disabled:
        break;

    case CaffeineMode::Enabled:
        keepScreenOn      = mSettings->Standard.KeepScreenOn;
        whenSessionLocked = mSettings->Standard.WhenSessionLocked;
        break;
    case CaffeineMode::Auto:
        keepScreenOn      = mSettings->Auto.KeepScreenOn;
        whenSessionLocked = mSettings->Auto.WhenSessionLocked;
        break;
    case CaffeineMode::Timer:
        keepScreenOn      = mSettings->Timer.KeepScreenOn;
        whenSessionLocked = mSettings->Timer.WhenSessionLocked;
        break;
    }

    auto needUpdate = true;
    if (mCaffeineMode != CaffeineMode::Disabled)
    {
        if (mSessionState == SessionState::Locked)
        {
            keepScreenOn = whenSessionLocked;
        }

        if (mCaffeineState == state && mKeepScreenOn == keepScreenOn)
        {
            needUpdate = false;
        }
    }
    else
    {
        if (mCaffeineState == state)
        {
            needUpdate = false;
        }
    }

    if (!needUpdate)
    {
        LOG_DEBUG("No need to update execution state, continuing");
        return;
    }

    // Update Execution State.
    mCaffeineState = state;
    mKeepScreenOn = keepScreenOn;

    auto flags = EXECUTION_STATE{ES_CONTINUOUS};
    if (mCaffeineState == CaffeineState::Active)
    {
        flags |= ES_SYSTEM_REQUIRED;

        if (keepScreenOn)
        {
            flags |= ES_DISPLAY_REQUIRED;
        }
    }

    if (!SetThreadExecutionState(flags))
    {
        LOG_ERROR("Failed to update execution state");
        return;
    }

    LOG_INFO("Updated execution state, State: {}, Display: {}", static_cast<int>(mCaffeineState), mKeepScreenOn);

    UpdateIcon();
    UpdateTip();
    UpdateJumpList();
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
    auto tip = std::wstring_view();

    switch (mCaffeineMode)
    {
    case CaffeineMode::Disabled:
        tip = mLang->Tip_DisabledInactive;
        break;

    case CaffeineMode::Enabled:
        tip = mLang->Tip_EnabledActive;
        break;

    case CaffeineMode::Auto:
        if (mCaffeineState == CaffeineState::Inactive)
        {
            tip = mLang->Tip_AutoInactive;
        }
        else
        {
            tip = mLang->Tip_AutoActive;
        }
        break;

    case CaffeineMode::Timer:
        if (mCaffeineState == CaffeineState::Inactive)
        {
            tip = mLang->Tip_TimerInactive;
        }
        else
        {
            tip = mLang->Tip_TimerActive;
        }
        break;
    }

    // No need to update.
    if (mNotifyIcon.GetTip() == tip.data())
    {
        return false;
    }

    const auto hr = mNotifyIcon.SetTip(tip);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to update notifyicon tip, error: {}", hr);
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

auto CaffeineApp::UpdateJumpList () -> bool
{
#if !defined(FEATURE_CAFFEINETAKE_JUMPLISTS)
    // TODO it would be got to clear lists
    return true;
#endif

    const auto exe = mExecutablePath.wstring();

    // TODO update icons of tasks
    // TODO translate
    const auto OptionDisableCaffeine = JumpListEntry(JumpListEntry::Type::Normal, L"Disable Caffeine", L"/task:DisableCaffeine", exe.c_str(), 0);
    const auto OptionEnableCaffeine  = JumpListEntry(JumpListEntry::Type::Normal, L"Enable Caffeine", L"/task:EnableCaffeine", exe.c_str(), 0);
    const auto OptionEnableAutoMode  = JumpListEntry(JumpListEntry::Type::Normal, L"Enable Auto Mode", L"/task:EnableAutoMode", exe.c_str(), 0);
    const auto OptionEnableTimerMode = JumpListEntry(JumpListEntry::Type::Normal, L"Enable Timer Mode", L"/task:EnableTimerMode", exe.c_str(), 0);
    const auto OptionSeparator       = JumpListEntry(JumpListEntry::Type::Separator, L"", L"", L"", 0);
    const auto OptionSettings        = JumpListEntry(JumpListEntry::Type::Normal, L"Settings", L"/task:Settings", exe.c_str(), 0);
    const auto OptionAbout           = JumpListEntry(JumpListEntry::Type::Normal, L"About", L"/task:About", exe.c_str(), 0);
    const auto OptionExit            = JumpListEntry(JumpListEntry::Type::Normal, L"Exit", L"/task:Exit", exe.c_str(), 0);

    auto list = std::vector<JumpListEntry>();

    if (!mShuttingDown)
    {
        switch (mCaffeineMode)
        {
        case CaffeineMode::Disabled:
            if (IsModeAvailable(CaffeineMode::Enabled)) {
                list.push_back(OptionEnableCaffeine);
            }
            if (IsModeAvailable(CaffeineMode::Auto)) {
                list.push_back(OptionEnableAutoMode);
            }
            if (IsModeAvailable(CaffeineMode::Timer)) {
                list.push_back(OptionEnableTimerMode);
            }
            break;
        case CaffeineMode::Enabled:
            if (IsModeAvailable(CaffeineMode::Auto)) {
                list.push_back(OptionEnableAutoMode);
            }
            if (IsModeAvailable(CaffeineMode::Timer)) {
                list.push_back(OptionEnableTimerMode);
            }
            if (IsModeAvailable(CaffeineMode::Disabled)) {
                list.push_back(OptionDisableCaffeine);
            }
            break;
        case CaffeineMode::Auto:
            if (IsModeAvailable(CaffeineMode::Enabled)) {
                list.push_back(OptionEnableCaffeine);
            }
            if (IsModeAvailable(CaffeineMode::Timer)) {
                list.push_back(OptionEnableTimerMode);
            }
            if (IsModeAvailable(CaffeineMode::Disabled)) {
                list.push_back(OptionDisableCaffeine);
            }
            break;
        case CaffeineMode::Timer:
            if (IsModeAvailable(CaffeineMode::Enabled)) {
                list.push_back(OptionEnableCaffeine);
            }
            if (IsModeAvailable(CaffeineMode::Auto)) {
                list.push_back(OptionEnableAutoMode);
            }
            if (IsModeAvailable(CaffeineMode::Disabled)) {
                list.push_back(OptionDisableCaffeine);
            }
            break;
        }

        list.push_back(OptionSeparator);
        
        if (IsFeatureAvailable(Feature::SettingsDialog)) {
            list.push_back(OptionSettings);
        }
        else {
            list.push_back(OptionAbout);
        }

        list.push_back(OptionExit);
    }
    else
    {
        // We want to clear the list on app shutdown.
        // TODO add option to start app with no mode change?
        if (IsModeAvailable(CaffeineMode::Enabled)) {
            list.push_back(OptionEnableCaffeine);
        }
        if (IsModeAvailable(CaffeineMode::Auto)) {
            list.push_back(OptionEnableAutoMode);
        }
        if (IsModeAvailable(CaffeineMode::Timer)) {
            list.push_back(OptionEnableTimerMode);
        }

        list.push_back(OptionSeparator);
        
        if (IsFeatureAvailable(Feature::SettingsDialog)) {
            list.push_back(OptionSettings);
        }
        else {
            list.push_back(OptionAbout);
        }
    }

    JumpList::Clear();
    const auto result = JumpList::Update(mExecutablePath, list);
    if (result)
    {
        LOG_INFO("Updated Jump List");
    }
    else
    {
        LOG_ERROR("Failed to update Jump List");
    }

    return result;
}

auto CaffeineApp::LoadSettings () -> void
{
    if (!mSettings->Load(mSettingsFilePath))
    {
        LOG_ERROR(L"Failed to load settings, using default values");
        mSettings = std::make_shared<Settings>();
    }
}

auto CaffeineApp::SaveSettings () -> void
{
    if (!mSettings->Save(mSettingsFilePath))
    {
        LOG_ERROR(L"Failed to save settings");
    }
}

auto CaffeineApp::LoadLang () -> void
{
    const auto langPath = mLangDirectory / (mSettings->General.LangId + L".json");
    if (!mLang->Load(langPath))
    {
        mLang = std::make_shared<Lang>();
        LOG_ERROR(L"Failed to load lang file, using default language '{}'", mLang->LangId);
    }
    else
    {
        mLang->LangId = mSettings->General.LangId;
        // TODO get name from langid
        //mLang->LangName = 
        LOG_INFO(L"Loaded language: '{}' ({})", mLang->LangId, mLang->LangName);
    }
}

auto CaffeineApp::ShowSettingsDialog () -> bool
{
#if defined(FEATURE_CAFFEINETAKE_SETTINGS_DIALOG)
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
#endif

    return true;
}

auto CaffeineApp::ShowAboutDialog () -> bool
{
    SINGLE_INSTANCE_GUARD();
    
    auto aboutDlg = AboutDialog();
    aboutDlg.Show(mNotifyIcon.Handle());

    return true;
}

auto CaffeineApp::IsModeAvailable (CaffeineMode mode) -> bool
{
    auto available = false;

    switch (mode)
    {
    case CaffeineMode::Disabled:
        available = true;
        break;

    case CaffeineMode::Enabled:
        if (mSettings->Standard.Enabled)
        {
            available = true;
        }
        break;

    case CaffeineMode::Auto:
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE)
        if (mSettings->Auto.Enabled)
        {
            available = true;
        }
#else
        available = false;
#endif
        break;

    case CaffeineMode::Timer:
#if defined(FEATURE_CAFFEINETAKE_TIMER_MODE)
        if (mSettings->Timer.Enabled)
        {
            available = true;
        }
#else
        available = false;
#endif
        break;
    }

    return available;
}

} // namespace CaffeineTake
