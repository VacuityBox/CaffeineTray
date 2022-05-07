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

#include "CaffeineAppSO.hpp"
#include "Logger.hpp"
#include "Scanner.hpp"
#include "Schedule.hpp"
#include "Settings.hpp"
#include "ThreadTimer.hpp"

#include <string_view>

namespace CaffeineTake {

enum class CaffeineMode : unsigned char
{
    Disabled = 0,  // TODO better name?
    Enabled  = 1,  // TODO better name?
    Auto     = 2,
    Timer    = 3
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

        LOG_TRACE("Started Disabled mode");
        return true;
    }

    auto Stop () -> bool override
    {
        LOG_TRACE("Stopped Disabled mode");
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

        LOG_TRACE("Started Enabled mode");
        return true;
    }

    auto Stop () -> bool override
    {
        mAppSO.DisableCaffeine();

        LOG_TRACE("Stopped Enabled mode");
        return true;
    }
};

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE)

// Define flag to indicate at least one trigger is set (excluding schedule trigger).
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_PROCESS) \
 || defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_WINDOW)  \
 || defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB)     \
 || defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH)

#define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_IS_SCANNER_REQUIRED
#endif

class AutoMode : public Mode
{
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_PROCESS)
    ProcessScanner   mProcessScanner;
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_WINDOW)
    WindowScanner    mWindowScanner;
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB)
    UsbDeviceScanner mUsbScanner;
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH)
    BluetoothScanner mBluetoothScanner;
#endif

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_IS_SCANNER_REQUIRED)
    bool             mScannerPreviousResult = false;
    ThreadTimer      mScannerTimer;

    auto ScannerTimerProc  (const StopToken& stop, const PauseToken& pause) -> bool;
#endif

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE)
    bool             mSchedulePreviousResult = false;
    ThreadTimer      mScheduleTimer;

    auto ScheduleTimerProc (const StopToken& stop, const PauseToken& pause) -> bool;
#endif

public:
    AutoMode (CaffeineAppSO app);

    auto Start () -> bool override;
    auto Stop  () -> bool override;
};

#endif // #if defined(FEATURE_CAFFEINETAKE_AUTO_MODE)

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

        // TOOD FIXME if interval is 0 dont start

        mAppSO.EnableCaffeine();
        mTimerThread.Start();

        LOG_TRACE("Started Timer mode");

        return true;
    }

    auto Stop () -> bool override
    {
        mTimerThread.Stop();
        mAppSO.DisableCaffeine();

        LOG_TRACE("Stopped Timer mode");

        return true;
    }
};

} // namespace CaffeineTake
