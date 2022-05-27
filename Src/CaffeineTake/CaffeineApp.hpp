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

#pragma once

#include "Config.hpp"

#include "AppInitInfo.hpp"
#include "CaffeineAppSO.hpp"
#include "CaffeineMode.hpp"
#include "CaffeineState.hpp"
#include "ForwardDeclaration.hpp"

#if defined(FEATURE_CAFFEINETAKE_IMMERSIVE_CONTEXT_MENU)
#   include <mni/ImmersiveNotifyIcon.hpp>
#else
#   include <mni/ClassicNotifyIcon.hpp>
#endif

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

// Custom messages.
constexpr auto WM_CAFFEINE_TAKE_UPDATE_EXECUTION_STATE  = (MNI_USER_MESSAGE_ID + 0);
constexpr auto WM_CAFFEINE_TAKE_SECOND_INSTANCE_MESSAGE = (MNI_USER_MESSAGE_ID + 1);

// Forward declaration of shared object.
class CaffeineAppSO;

class CaffeineApp final
{
    friend class CaffeineAppSO;

    HINSTANCE          mInstanceHandle;
    mni::NotifyIcon    mNotifyIcon;
    mni::ThemeInfo     mThemeInfo;
    CaffeineAppSO      mAppSO;
    CaffeineMode       mCaffeineMode;
    CaffeineState      mCaffeineState;
    bool               mKeepScreenOn;
    bool               mInitialized;
    bool               mShuttingDown;
    bool               mIsStopping;
    bool               mUpdatedByES;
    SessionState       mSessionState;
    fs::path           mExecutablePath;
    fs::path           mSettingsFilePath;
    fs::path           mCustomIconsPath;
    fs::path           mCustomSoundsPath;
    fs::path           mLangDirectory;
    int                mDpi;

    SettingsPtr        mSettings;
    LangPtr            mLang;
    CaffeineIconsPtr   mIcons;
    CaffeineSoundsPtr  mSounds;

    Mode*              mModePtr;
    DisabledMode       mDisabledMode;
    StandardMode       mStandardMode;
    AutoMode           mAutoMode;
    TimerMode          mTimerMode;

    auto OnCreate            ()                     -> void;
    auto OnDestroy           ()                     -> void;
    auto OnClick             (int x, int y)         -> void;
    auto OnContextMenuOpen   ()                     -> void;
    auto OnContextMenuSelect (int selectedItem)     -> void;
    auto OnThemeChange       (mni::ThemeInfo ti)    -> void;
    auto OnDpiChange         (int dpi)              -> void;
    auto OnCustomMessage     (UINT, WPARAM, LPARAM) -> void;
    auto OnSystemMessage     (UINT, WPARAM, LPARAM) -> bool;
   
    // This sends signal to enable/disable caffeine.
    auto EnableCaffeine  () -> bool;
    auto DisableCaffeine () -> bool;

    // Change mode. Messages received from controls.
    auto ToggleCaffeineMode () -> void;
    auto SetCaffeineMode    (CaffeineMode mode) -> void;
    
    auto StartMode () -> void;
    auto StopMode  () -> void;

    auto LoadMode () -> bool;
    auto SaveMode () -> bool;

    // Main update method. This change ui/es.
    auto UpdateExecutionState  (CaffeineState state) -> void;
    auto RefreshExecutionState () -> void;
    
    auto UpdateIcon     () -> bool;
    auto UpdateTip      () -> bool;
    auto UpdateAppIcon  () -> void;
    auto UpdateJumpList () -> bool;

    auto ShowNotificationBalloon () -> void;
    auto PlayNotificationSound   () -> void;

    auto ProcessTask (unsigned int msg) -> bool;

    auto LoadSettings () -> void;
    auto SaveSettings () -> void;

    auto LoadLang     () -> void;

    auto ShowSettingsDialog () -> bool;
    auto ShowAboutDialog    () -> bool;

    auto IsModeAvailable (CaffeineMode mode) -> bool;

    CaffeineApp            (const CaffeineApp&) = delete;
    CaffeineApp& operator= (const CaffeineApp&) = delete;
    CaffeineApp            (CaffeineApp&&)      = delete;
    CaffeineApp& operator= (CaffeineApp&&)      = delete;

public:
    CaffeineApp  (const AppInitInfo& info);
    ~CaffeineApp ();

    auto Init     (const AppInitInfo& info) -> bool;
    auto MainLoop () -> int;

    static auto SendMessageToMainInstance (UINT uMsg, WPARAM wParam, LPARAM lParam) -> bool;
};

} // namespace CaffeineTake
