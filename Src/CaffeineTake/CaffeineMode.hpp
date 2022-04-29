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

#include "CaffeineAppSO.hpp"
#include "Scanner.hpp"
#include "Schedule.hpp"
#include "Settings.hpp"
#include "ThreadTimer.hpp"

#include <string_view>

namespace CaffeineTake {

enum class CaffeineMode : unsigned char
{
    Disabled,
    Enabled,
    Auto
};

constexpr auto CaffeineModeToString (CaffeineMode mode) -> std::wstring_view
{
    switch (mode)
    {
    case CaffeineMode::Disabled: return L"Disabled";
    case CaffeineMode::Enabled:  return L"Enabled";
    case CaffeineMode::Auto:     return L"Auto";
    }

    return L"Invalid CaffeineMode";
}

class DisabledMode
{
public:
    auto Start (CaffeineAppSO* app) -> bool
    {
        app->DisableCaffeine();
        return true;
    }

    auto Stop (CaffeineAppSO* app) -> bool
    {
        return true;
    }
};

class EnabledMode
{
public:
    auto Start (CaffeineAppSO* app) -> bool
    {
        app->EnableCaffeine();
        return true;
    }

    auto Stop (CaffeineAppSO* app) -> bool
    {
        app->DisableCaffeine();
        return true;
    }
};

class AutoMode
{
    CaffeineAppSO* mAppPtr;
    SettingsPtr    mSettingsPtr;
    
    ProcessScanner   mProcessScanner;
    WindowScanner    mWindowScanner;
    UsbDeviceScanner mUsbScanner;

    bool           mScannerPreviousResult;
    bool           mSchedulePreviousResult;

    ThreadTimer    mScannerTimer;
    ThreadTimer    mScheduleTimer;

    auto ScannerTimerProc  () -> void;
    auto ScheduleTimerProc () -> void;

public:
    AutoMode (CaffeineAppSO* app, SettingsPtr settings)
        : mAppPtr                 (app)
        , mSettingsPtr            (settings)
        , mProcessScanner         ()
        , mWindowScanner          ()
        , mScannerTimer           (std::bind(&AutoMode::ScannerTimerProc, this))
        , mScheduleTimer          (std::bind(&AutoMode::ScheduleTimerProc, this), std::chrono::milliseconds(1000))
        , mScannerPreviousResult  (false)
        , mSchedulePreviousResult (false)
    {
    }

    auto Start (CaffeineAppSO* app) -> bool
    {
        app->DisableCaffeine();

        mScannerTimer.Interval(std::chrono::milliseconds(mSettingsPtr->Auto.ScanInterval));

        mScheduleTimer.Start();
        mScannerTimer.Start();

        return true;
    }

    auto Stop (CaffeineAppSO* app) -> bool
    {
        mScannerTimer.Stop();
        mScheduleTimer.Stop();

        app->DisableCaffeine();

        return true;
    }
};

} // namespace CaffeineTake
