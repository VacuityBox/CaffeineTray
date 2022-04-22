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
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "AppInitInfo.hpp"
#include "ExecutionState.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "IconPack.hpp"
#include "Scanner.hpp"
#include "Settings.hpp"
#include "Timer.hpp"

#include <mni/NotifyIcon.hpp>

#include <filesystem>
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

class CaffeineTakeApp final
{
    std::shared_ptr<Settings> mSettings;

    mni::NotifyIcon mNotifyIcon;
    HINSTANCE       mInstanceHandle;
    bool            mLightTheme;
    bool            mInitialized;    
    bool            mSessionLocked;
    fs::path        mSettingsFilePath;
    ExecutionState  mCaffeine;
    std::mutex      mCaffeineMutex;
    ProcessScanner  mProcessScanner;
    WindowScanner   mWindowScanner;
    Timer           mScannerTimer;
    fs::path        mCustomIconsPath;
    IconPack        mIconPack;

    auto OnCreate            ()                     -> void;
    auto OnDestroy           ()                     -> void;
    auto OnClick             (int x, int y)         -> void;
    auto OnContextMenuOpen   ()                     -> void;
    auto OnContextMenuSelect (int selectedItem)     -> void;
    auto OnThemeChange       (mni::ThemeInfo ti)    -> void;
    auto OnDpiChange         (int dpi)              -> void;
    auto OnSystemMessage     (UINT, WPARAM, LPARAM) -> bool;
   
    auto EnableCaffeine        () -> bool;
    auto DisableCaffeine       () -> bool;
    auto UpdateExecutionState  (bool activate = false) -> void;
    auto ToggleCaffeineMode    () -> void;
    auto SetCaffeineMode       (CaffeineMode mode) -> void;

    auto UpdateIcon      (bool autoActive = false) -> bool;
    auto UpdateAppIcon   ()          -> void;
    auto LoadIconHelper  (UINT icon) -> HICON;
    auto LoadSquaredIcon (UINT icon) -> HICON;
    auto LoadCustomIcon  (UINT icon) -> HICON;

    auto LoadSettings () -> bool;
    auto SaveSettings () -> bool;

    auto ResetTimer  () -> bool;
    auto TimerUpdate () -> void;

    auto ShowCaffeineSettings () -> bool;
    auto ShowAboutDialog      () -> bool;

    auto Log (std::string message) -> void;
    
    CaffeineTakeApp (const CaffeineTakeApp&) = delete;
    CaffeineTakeApp (CaffeineTakeApp&&)      = delete;
    CaffeineTakeApp& operator = (const CaffeineTakeApp&) = delete;
    CaffeineTakeApp& operator = (CaffeineTakeApp&&)      = delete;

public:
    CaffeineTakeApp  (const AppInitInfo& info);
    ~CaffeineTakeApp ();

    auto Init     () -> bool;
    auto MainLoop () -> int;
};

} // namespace CaffeineTake
