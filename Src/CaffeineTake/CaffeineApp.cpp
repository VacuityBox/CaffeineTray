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

#include "PCH.hpp"
#include "CaffeineApp.hpp"

#include "CaffeineIcons.hpp"
#include "CaffeineSounds.hpp"
#include "Dialogs/AboutDialog.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "JumpList.hpp"
#include "Lang.hpp"
#include "Logger.hpp"
#include "Resource.hpp"
#include "Settings.hpp"
#include "Tasks.hpp"
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

// Window Title and Class Name.
constexpr auto CAFFEINE_TAKE_WINDOW_TITLE = L"CaffeineTake_WndClass";
constexpr auto CAFFEINE_TAKE_CLASS_NAME   = L"CaffeineTake_InvisibleWindow";

CaffeineApp::CaffeineApp (const AppInitInfo& info)
    : mSettings           (std::make_shared<Settings>())
    , mLang               (std::make_shared<Lang>())
    , mExecutablePath     (info.ExecutablePath)
    , mSettingsFilePath   (info.SettingsPath)
    , mCustomIconsPath    (info.DataDirectory / "Icons" / "")
    , mCustomSoundsPath   (info.DataDirectory / "Sounds" / "")
    , mLangDirectory      (info.DataDirectory / "Lang" / "")
    , mInstanceHandle     (info.InstanceHandle)
    , mInitialized        (false)
    , mShuttingDown       (false)
    , mIsStopping         (false)
    , mUpdatedByES        (false)
    , mSessionState       (SessionState::Unlocked)
    , mNotifyIcon         ()
    , mThemeInfo          (mni::ThemeInfo::Detect())
    , mIcons              (std::make_shared<CaffeineIcons>(info.InstanceHandle, mCustomIconsPath))
    , mSounds             (std::make_shared<CaffeineSounds>(info.InstanceHandle, mCustomSoundsPath))
    , mCaffeineState      (CaffeineState::Inactive)
    , mCaffeineMode       (CaffeineMode::Disabled)
    , mKeepScreenOn       (false)
    , mAppSO              (this)
    , mDisabledMode       (mAppSO)
    , mStandardMode       (mAppSO)
    , mAutoMode           (mAppSO)
    , mTimerMode          (mAppSO)
    , mDpi                (96)
    , mModePtr            (nullptr)
{
}

CaffeineApp::~CaffeineApp()
{
    mShuttingDown = true;
    SetCaffeineMode(CaffeineMode::Disabled);
    CoUninitialize();
}

auto CaffeineApp::Init (const AppInitInfo& info) -> bool
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

    // For Jump Lists and shortcut.
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
            .windowTitle = CAFFEINE_TAKE_WINDOW_TITLE,
            .className   = CAFFEINE_TAKE_CLASS_NAME
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

        mIcons->Load(mSettings->General.IconPack, mThemeInfo.IsDark() ? CaffeineIcons::SystemTheme::Light : CaffeineIcons::SystemTheme::Dark, w, h, mSettings);
    }

    // Load sounds.
    {
        mSounds->Load(mSettings->General.SoundPack);
    }

    // Load language.
    {
        LoadLang();
    }

    // Update mode, icons, execution state, tip.
    {
        // If app was launched from jump list we that to set mode.
        if (!ProcessTask(info.Args.Task.MessageId))
        {
            if (!LoadMode())
            {
                LOG_INFO("Writing default mode to registry");
                SaveMode();
            }
            
            SetCaffeineMode(mCaffeineMode);
        }

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
        if (IsModeAvailable(CaffeineMode::Standard)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_CAFFEINE, mLang->ContextMenu_EnableStandard.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Auto)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_AUTO, mLang->ContextMenu_EnableAuto.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Timer)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_TIMER, mLang->ContextMenu_EnableTimer.c_str());
        }
        break;
    case CaffeineMode::Standard:
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
        if (IsModeAvailable(CaffeineMode::Standard)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_CAFFEINE, mLang->ContextMenu_EnableStandard.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Timer)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_TIMER, mLang->ContextMenu_EnableTimer.c_str());
        }
        if (IsModeAvailable(CaffeineMode::Disabled)) {
            AppendMenuW(hMenu, MF_STRING, IDM_DISABLE_CAFFEINE, mLang->ContextMenu_DisableCaffeine.c_str());
        }
        break;
    case CaffeineMode::Timer:
        if (IsModeAvailable(CaffeineMode::Standard)) {
            AppendMenuW(hMenu, MF_STRING, IDM_ENABLE_CAFFEINE, mLang->ContextMenu_EnableStandard.c_str());
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
        SetCaffeineMode(CaffeineMode::Standard);
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
    mIcons->Load(mSettings->General.IconPack, mThemeInfo.IsDark() ? CaffeineIcons::SystemTheme::Light : CaffeineIcons::SystemTheme::Dark, w, h, mSettings);

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
    mIcons->Load(mSettings->General.IconPack, mThemeInfo.IsDark() ? CaffeineIcons::SystemTheme::Light : CaffeineIcons::SystemTheme::Dark, w, h, mSettings);

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

    case WM_CAFFEINE_TAKE_SECOND_INSTANCE_MESSAGE:
        LOG_INFO("Received message from jumplist {}", static_cast<unsigned int>(wParam));
        ProcessTask(static_cast<unsigned int>(wParam));
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
        mode = CaffeineMode::Standard;
        break;

    case CaffeineMode::Standard:
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

    case CaffeineMode::Standard:
        if (IsModeAvailable(CaffeineMode::Standard)) {
            nextMode = &mStandardMode;
        }
        else {
            LOG_ERROR("Mode 'Standard' is not available");
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
    mIsStopping = true;
    StopMode();
    mIsStopping = false;
    mUpdatedByES = false;

    // Start new one.
    mCaffeineMode = mode;
    mModePtr = nextMode;

    if (mModePtr) {
        LOG_INFO(L"Setting CaffeineMode to {}", mModePtr->GetName());
    }
    StartMode();

    if (!mUpdatedByES)
    {
        UpdateIcon();
        UpdateTip();
        UpdateJumpList();
        ShowNotificationBalloon();
        PlayNotificationSound();
    }

    if (!mShuttingDown)
    {
        SaveMode();
    }
}

auto CaffeineApp::StartMode () -> void
{
    if (mModePtr)
    {
        mModePtr->Start();
    }
}

auto CaffeineApp::StopMode () -> void
{
    if (mModePtr)
    {
        mModePtr->Stop();
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

    case CaffeineMode::Standard:
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

    mUpdatedByES = true;

    UpdateIcon();
    UpdateTip();
    UpdateJumpList();
    ShowNotificationBalloon();
    PlayNotificationSound();
}

auto CaffeineApp::RefreshExecutionState () -> void
{
    UpdateExecutionState(mCaffeineState);
}

auto CaffeineApp::UpdateIcon() -> bool
{
    auto icon = mModePtr->GetIcon(mCaffeineState);

    // No need to update.
    if (mNotifyIcon.GetIcon() == icon)
    {
        return false;
    }

    if (FAILED(mNotifyIcon.SetIcon(icon, mni::NotifyIcon::RDP::Manual)))
    {
        LOG_ERROR("Failed to update notifyicon icon");
        return false;
    }

    LOG_INFO("Updated notifyicon icon");
    return true;
}

auto CaffeineApp::UpdateTip() -> bool
{
    auto tip = mModePtr->GetTip(mCaffeineState);

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
    const auto OptionDisableCaffeine = JumpListEntry(JumpListEntry::Type::Normal, mLang->Task_DisableCaffeine,    TASK_DISBALE_CAFFEINE,     exe.c_str(), 0);
    const auto OptionEnableCaffeine  = JumpListEntry(JumpListEntry::Type::Normal, mLang->Task_EnableStandardMode, TASK_ENABLE_STANDARD_MODE, exe.c_str(), 1);
    const auto OptionEnableAutoMode  = JumpListEntry(JumpListEntry::Type::Normal, mLang->Task_EnableAutoMode,     TASK_ENABLE_AUTO_MODE,     exe.c_str(), 2);
    const auto OptionEnableTimerMode = JumpListEntry(JumpListEntry::Type::Normal, mLang->Task_EnableTimerMode,    TASK_ENABLE_TIMER_MODE,    exe.c_str(), 3);
    const auto OptionSettings        = JumpListEntry(JumpListEntry::Type::Normal, mLang->Task_Settings,           TASK_SHOW_SETTINGS_DIALOG, exe.c_str(), 4);
    const auto OptionAbout           = JumpListEntry(JumpListEntry::Type::Normal, mLang->Task_About,              TASK_SHOW_ABOUT_DIALOG,    exe.c_str(), 5);
    const auto OptionExit            = JumpListEntry(JumpListEntry::Type::Normal, mLang->Task_Exit,               TASK_EXIT,                 exe.c_str(), 6);
    const auto OptionSeparator       = JumpListEntry(JumpListEntry::Type::Separator, L"", L"", L"", 0);

    auto list = std::vector<JumpListEntry>();

    if (!mShuttingDown)
    {
        switch (mCaffeineMode)
        {
        case CaffeineMode::Disabled:
            if (IsModeAvailable(CaffeineMode::Standard)) {
                list.push_back(OptionEnableCaffeine);
            }
            if (IsModeAvailable(CaffeineMode::Auto)) {
                list.push_back(OptionEnableAutoMode);
            }
            if (IsModeAvailable(CaffeineMode::Timer)) {
                list.push_back(OptionEnableTimerMode);
            }
            break;
        case CaffeineMode::Standard:
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
            if (IsModeAvailable(CaffeineMode::Standard)) {
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
            if (IsModeAvailable(CaffeineMode::Standard)) {
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
        if (IsModeAvailable(CaffeineMode::Standard)) {
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

    //JumpList::Clear();
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

auto CaffeineApp::ShowNotificationBalloon () -> void
{
    if (mSettings->General.ShowNotifications && !mIsStopping)
    {
        auto title = L"";
        auto text  = L"";

        switch (mCaffeineMode)
        {
        case CaffeineMode::Disabled:
            title = L"Standard Mode";
            text = L"Caffeine Inactive";
            break;
        case CaffeineMode::Standard:
            title = L"Standard Mode";
            text = L"Caffeine Active";
            break;
        case CaffeineMode::Auto:
            title = L"Auto Mode";
            if (mCaffeineState == CaffeineState::Inactive)
            {
                text = L"Caffeine Inactive";
            }
            else
            {
                text = L"Caffeine Active";
            }
            break;
        case CaffeineMode::Timer:
            title = L"Timer Mode";
            if (mCaffeineState == CaffeineState::Inactive)
            {
                text = L"Caffeine Inactive";
            }
            else
            {
                text = L"Caffeine Active";
            }
            break;

        }
        
        //mNotifyIcon.RemoveBalloonNotification();

        const auto flags = mni::BalloonFlags::Realtime | mni::BalloonFlags::RespectQuietTime;
        mNotifyIcon.SendBalloonNotification(title, text, mni::BalloonIconType::NoIcon, NULL, flags);
    }
}

auto CaffeineApp::PlayNotificationSound () -> void
{
    // TODO respect quiet mode
    if (mSettings->General.PlayNotificationSound)
    {
        switch (mCaffeineState)
        {
        case CaffeineState::Active:
            mSounds->PlayActivateSound();
            break;
        case CaffeineState::Inactive:
            mSounds->PlayDeactivateSound();
            break;
        }
    }
}

auto CaffeineApp::ProcessTask (unsigned int msg) -> bool
{
    auto modeChanged = false;

    switch (msg)
    {
    case TASK_DISBALE_CAFFEINE.MessageId:
        SetCaffeineMode(CaffeineMode::Disabled);
        modeChanged = true;
        break;
    case TASK_ENABLE_STANDARD_MODE.MessageId:
        SetCaffeineMode(CaffeineMode::Standard);
        modeChanged = true;
        break;
    case TASK_ENABLE_AUTO_MODE.MessageId:
        SetCaffeineMode(CaffeineMode::Auto);
        modeChanged = true;
        break;
    case TASK_ENABLE_TIMER_MODE.MessageId:
        SetCaffeineMode(CaffeineMode::Timer);
        modeChanged = true;
        break;
    case TASK_SHOW_SETTINGS_DIALOG.MessageId:
        ShowSettingsDialog();
        break;
    case TASK_SHOW_ABOUT_DIALOG.MessageId:
        ShowAboutDialog();
        break;
    case TASK_EXIT.MessageId:
        mNotifyIcon.Quit();
        break;
    }

    return modeChanged;
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

    case CaffeineMode::Standard:
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

auto CaffeineApp::SendMessageToMainInstance (UINT uMsg, WPARAM wParam, LPARAM lParam) -> bool
{    
    auto hWnd = FindWindowW(CAFFEINE_TAKE_CLASS_NAME, CAFFEINE_TAKE_WINDOW_TITLE);
    if (hWnd)
    {
        SendMessageW(hWnd, uMsg, wParam, lParam);
        return true;
    }

    return false;
}

} // namespace CaffeineTake
