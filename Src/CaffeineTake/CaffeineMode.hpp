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
    Auto,
    Timer
};

constexpr auto CaffeineModeToString (CaffeineMode mode) -> std::wstring_view
{
    switch (mode)
    {
    case CaffeineMode::Disabled: return L"Disabled";
    case CaffeineMode::Enabled:  return L"Enabled";
    case CaffeineMode::Auto:     return L"Auto";
    case CaffeineMode::Timer:    return L"Timer";
    }

    return L"Invalid CaffeineMode";
}

class Mode
{
protected:
    CaffeineAppSO mAppSO = nullptr;

public:
    Mode (CaffeineAppSO app)
        : mAppSO (app)
    {
    }

    virtual auto Start () -> bool = 0;
    virtual auto Stop  () -> bool = 0;

    //virtual auto OnCustomMessage (UINT, WPARAM, LPARAM) -> bool = 0;
};

class DisabledMode : public Mode
{
public:
    DisabledMode (CaffeineAppSO app)
        : Mode (app)
    {
    }

    auto Start () -> bool override
    {
        mAppSO.DisableCaffeine();

        spdlog::trace("Started Disabled mode");
        return true;
    }

    auto Stop () -> bool override
    {
        spdlog::trace("Stopped Disabled mode");
        return true;
    }
};

class EnabledMode : public Mode
{
public:
    EnabledMode (CaffeineAppSO app)
        : Mode (app)
    {
    }

    auto Start () -> bool override
    {
        mAppSO.EnableCaffeine();

        spdlog::trace("Started Enabled mode");
        return true;
    }

    auto Stop () -> bool override
    {
        mAppSO.DisableCaffeine();

        spdlog::trace("Stopped Enabled mode");
        return true;
    }
};

class AutoMode : public Mode
{
    ProcessScanner   mProcessScanner;
    WindowScanner    mWindowScanner;
    UsbDeviceScanner mUsbScanner;
    BluetoothScanner mBluetoothScanner;

    bool             mScannerPreviousResult;
    bool             mSchedulePreviousResult;

    ThreadTimer      mScannerTimer;
    ThreadTimer      mScheduleTimer;

    auto ScannerTimerProc  (const StopToken& stop, const PauseToken& pause) -> bool;
    auto ScheduleTimerProc (const StopToken& stop, const PauseToken& pause) -> bool;

public:
    AutoMode (CaffeineAppSO app)
        : Mode                    (app)
        , mProcessScanner         ()
        , mWindowScanner          ()
        , mScannerTimer
            ( std::bind(&AutoMode::ScannerTimerProc, this, std::placeholders::_1, std::placeholders::_2)
            , ThreadTimer::Interval(1000)
            , false
            , true
            )
        , mScheduleTimer
            ( std::bind(&AutoMode::ScheduleTimerProc, this, std::placeholders::_1, std::placeholders::_2)
            , ThreadTimer::Interval(1000)
            , false
            , true
            )
        , mScannerPreviousResult  (false)
        , mSchedulePreviousResult (false)
    {
    }

    auto Start () -> bool override
    {
        mAppSO.DisableCaffeine();

        const auto settingsPtr = mAppSO.GetSettings();
        if (settingsPtr)
        {
            mScannerTimer.SetInterval(std::chrono::milliseconds(settingsPtr->Auto.ScanInterval));
        }

        mScheduleTimer.Start();
        mScannerTimer.Start();

        spdlog::trace("Started Auto mode");

        return true;
    }

    auto Stop () -> bool override
    {
        mScannerTimer.Stop();
        mScheduleTimer.Stop();

        mAppSO.DisableCaffeine();

        spdlog::trace("Stopped Auto mode");

        return true;
    }
};

class TimerMode : public Mode
{
    ThreadTimer mTimerThread;

    auto TimerProc (const StopToken& stop, const PauseToken& pause) -> bool
    {
        mAppSO.DisableCaffeine();

        return false;
    }

public:
    TimerMode (CaffeineAppSO app)
        : Mode         (app)
        , mTimerThread
            ( std::bind(&TimerMode::TimerProc, this, std::placeholders::_1, std::placeholders::_2)
            , ThreadTimer::Interval(1000)
            , false
            , false
            )
    {
    }

    auto Start () -> bool override
    {
        const auto settingsPtr = mAppSO.GetSettings();
        if (settingsPtr)
        {
            mTimerThread.SetInterval(std::chrono::milliseconds(settingsPtr->Timer.Interval));
        }

        mAppSO.EnableCaffeine();
        mTimerThread.Start();

        spdlog::trace("Started Timer mode");

        return true;
    }

    auto Stop () -> bool override
    {
        mTimerThread.Stop();
        mAppSO.DisableCaffeine();

        spdlog::trace("Stopped Timer mode");

        return true;
    }
};

} // namespace CaffeineTake
