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

#include "CaffeineMode.hpp"

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE)

#include <chrono>

namespace CaffeineTake {

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_IS_SCANNER_REQUIRED)
auto AutoMode::ScannerTimerProc (const StopToken& stop, const PauseToken& pause) -> bool
{
    const auto settingsPtr = mAppSO.GetSettings();
    if (!settingsPtr)
    {
        return false;
    }

    auto scannerResult = false;
    
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_PROCESS)
    if (!scannerResult)
    {
        scannerResult = mProcessScanner.Run(settingsPtr, stop, pause);
    }
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_WINDOW)
    if (!scannerResult)
    {
        scannerResult = mWindowScanner.Run(settingsPtr, stop, pause);
    }
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB)
    if (!scannerResult)
    {
        scannerResult = mUsbScanner.Run(settingsPtr, stop, pause);
    }
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH)
    if (!scannerResult)
    {
        scannerResult = mBluetoothScanner.Run(settingsPtr, stop, pause);
    }
#endif

    // Only if there is state change.
    if (scannerResult != mScannerPreviousResult)
    {
        if (scannerResult)
        {
            mAppSO.EnableCaffeine();
        }
        else
        {
            mAppSO.DisableCaffeine();
        }

        mScannerPreviousResult = scannerResult;
    }

    return true;
}
#endif

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE)
auto AutoMode::ScheduleTimerProc (const StopToken& stop, const PauseToken& pause) -> bool
{
    const auto settingsPtr = mAppSO.GetSettings();
    if (!settingsPtr)
    {
        return false;
    }

    // Scan processes and windows if no process found.
    auto scheduleResult = Schedule::CheckSchedule(
        settingsPtr->Auto.ScheduleEntries, std::chrono::system_clock::now()
    );

    // Only if there is state change.
    if (scheduleResult != mSchedulePreviousResult)
    {
        if (scheduleResult)
        {
            mAppSO.EnableCaffeine();
        }
        else
        {
            mAppSO.DisableCaffeine();
        }

        mSchedulePreviousResult = scheduleResult;
    }

    // TODO it would be better if we first check schedule and the run scanner
    return true;
}
#endif

AutoMode::AutoMode (CaffeineAppSO app)
    : Mode (app)
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_IS_SCANNER_REQUIRED)
    , mScannerTimer
        ( std::bind(&AutoMode::ScannerTimerProc, this, std::placeholders::_1, std::placeholders::_2)
        , ThreadTimer::Interval(1000)
        , false
        , true
        )
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE)
    , mScheduleTimer
        ( std::bind(&AutoMode::ScheduleTimerProc, this, std::placeholders::_1, std::placeholders::_2)
        , ThreadTimer::Interval(1000)
        , false
        , true
        )
#endif
{
}

auto AutoMode::Start () -> bool
{
    mAppSO.DisableCaffeine();

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE)
    mSchedulePreviousResult = false;
    mScheduleTimer.Start();
#endif

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_IS_SCANNER_REQUIRED)
    const auto settingsPtr = mAppSO.GetSettings();
    if (settingsPtr)
    {
        mScannerTimer.SetInterval(std::chrono::milliseconds(settingsPtr->Auto.ScanInterval));
    }

    mScannerPreviousResult = false;
    mScannerTimer.Start();
#endif

    LOG_TRACE("Started Auto mode");

    return true;
}

auto AutoMode::Stop () -> bool
{
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE)
    mScheduleTimer.Stop();
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_IS_SCANNER_REQUIRED)
    mScannerTimer.Stop();
#endif

    mAppSO.DisableCaffeine();

    LOG_TRACE("Stopped Auto mode");

    return true;
}

} // namespace CaffeineTake

#endif // #if defined(FEATURE_CAFFEINETAKE_AUTO_MODE)
