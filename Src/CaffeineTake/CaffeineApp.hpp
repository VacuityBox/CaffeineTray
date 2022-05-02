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

#include "AppInitInfo.hpp"
#include "CaffeineAppSO.hpp"
#include "CaffeineIcons.hpp"
#include "CaffeineMode.hpp"
#include "CaffeineState.hpp"
#include "Settings.hpp"

#include <mni/NotifyIcon.hpp>

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
constexpr auto WM_CAFFEINE_TAKE_UPDATE_EXECUTION_STATE = (MNI_USER_MESSAGE_ID + 0);

// Forward declaration of shared object.
class CaffeineAppSO;

class CaffeineApp final
{
    friend class CaffeineAppSO;

    HINSTANCE        mInstanceHandle;
    mni::NotifyIcon  mNotifyIcon;
    mni::ThemeInfo   mThemeInfo;
    SettingsPtr      mSettings;
    CaffeineIcons    mIcons;
    CaffeineAppSO    mAppSO;
    CaffeineMode     mCaffeineMode;
    CaffeineState    mCaffeineState;
    bool             mKeepDisplayOn;
    bool             mInitialized;
    SessionState     mSessionState;
    fs::path         mCustomIconsPath;
    fs::path         mSettingsFilePath;
    int              mDpi;

    Mode*            mCurrentMode;
    DisabledMode     mDisabledMode;
    EnabledMode      mEnabledMode;
    AutoMode         mAutoMode;
    TimerMode        mTimerMode;

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
    
    auto UpdateIcon    () -> bool;
    auto UpdateTip     () -> bool;
    auto UpdateAppIcon () -> void;

    auto LoadSettings () -> bool;
    auto SaveSettings () -> bool;

    auto ShowSettingsDialog () -> bool;
    auto ShowAboutDialog    () -> bool;

    CaffeineApp            (const CaffeineApp&) = delete;
    CaffeineApp& operator= (const CaffeineApp&) = delete;
    CaffeineApp            (CaffeineApp&&)      = delete;
    CaffeineApp& operator= (CaffeineApp&&)      = delete;

public:
    CaffeineApp  (const AppInitInfo& info);
    ~CaffeineApp ();

    auto Init     () -> bool;
    auto MainLoop () -> int;
};

} // namespace CaffeineTake
