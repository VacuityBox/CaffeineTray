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
#include "Config.hpp"
#include "../CaffeineMode.hpp"

#include "Lang.hpp"
#include "Logger.hpp"
#include "Settings.hpp"

namespace CaffeineTake {

auto AutoMode::ScannerTimerProc (const StopToken& stop, const PauseToken& pause) -> bool
{
    const auto settingsPtr = mAppSO.GetSettings();
    if (!settingsPtr)
    {
        return true;
    }

    // If schedule is hit we don't need to scan.
    {
        auto lockGuard = std::lock_guard<std::mutex>(mScanMutex);
        if (mScheduleResult)
        {
            return true;
        }
    }

    auto scannerResult = false;
    
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_PROCESS)
    if (!scannerResult && settingsPtr->Auto.TriggerProcess.Enabled)
    {
        scannerResult = mProcessScanner.Run(settingsPtr, stop, pause);
    }
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_WINDOW)
    if (!scannerResult && settingsPtr->Auto.TriggerWindow.Enabled)
    {
        scannerResult = mWindowScanner.Run(settingsPtr, stop, pause);
    }
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB)
    if (!scannerResult && settingsPtr->Auto.TriggerUsb.Enabled)
    {
        scannerResult = mUsbScanner.Run(settingsPtr, stop, pause);
    }
#endif
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH)
    if (!scannerResult && settingsPtr->Auto.TriggerBluetooth.Enabled)
    {
        scannerResult = mBluetoothScanner.Run(settingsPtr, stop, pause);
    }
#endif

    // Only if there is state change.
    if (scannerResult != mScannerResult)
    {
        if (scannerResult)
        {
            mAppSO.EnableCaffeine();
        }
        else
        {
            mAppSO.DisableCaffeine();
        }

        mScannerResult = scannerResult;
    }

    if (stop)
    {
        return false;
    }

    return true;
}

auto AutoMode::ScheduleTimerProc (const StopToken& stop, const PauseToken& pause) -> bool
{
    const auto settingsPtr = mAppSO.GetSettings();
    if (!settingsPtr)
    {
        return true;
    }

    auto scheduleResult = false;

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE)
    if (settingsPtr->Auto.TriggerSchedule.Enabled)
    {
        scheduleResult = Schedule::CheckSchedule(
            settingsPtr->Auto.TriggerSchedule.ScheduleEntries, std::chrono::system_clock::now()
        );
    }
#endif

    // Only if there is state change.
    {
        auto lockGuard = std::lock_guard<std::mutex>(mScanMutex);
        if (scheduleResult != mScheduleResult)
        {
            if (scheduleResult)
            {
                mAppSO.EnableCaffeine();
            }
            else
            {
                mAppSO.DisableCaffeine();
            }

            mScheduleResult = scheduleResult;
        }
    }

    return true;
}

AutoMode::AutoMode (CaffeineAppSO app)
    : Mode (app)
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
{
}

auto AutoMode::Start () -> bool
{
    mAppSO.DisableCaffeine();

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE)
    mScheduleResult = false;
    mScheduleTimer.Start();
#endif

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_PROCESS) \
 || defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_WINDOW) \
 || defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB) \
 || defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH)
    const auto settingsPtr = mAppSO.GetSettings();
    if (settingsPtr)
    {
        mScannerTimer.SetInterval(std::chrono::milliseconds(settingsPtr->Auto.ScanInterval));
    }

    mScannerResult = false;
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
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_PROCESS) \
 || defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_WINDOW) \
 || defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB) \
 || defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH)
    mScannerTimer.Stop();
#endif

    mAppSO.DisableCaffeine();

    LOG_TRACE("Stopped Auto mode");

    return true;
}

auto AutoMode::GetIcon (CaffeineState state) const -> const HICON
{
    auto icons = mAppSO.GetIcons();

    switch (state)
    {
    case CaffeineTake::CaffeineState::Inactive: return icons->CaffeineAutoInactive;
    case CaffeineTake::CaffeineState::Active:   return icons->CaffeineAutoActive;
    }

    return NULL;
}

auto AutoMode::GetTip (CaffeineState state) const -> const std::wstring&
{
    auto lang = mAppSO.GetLang();

    switch (state)
    {
    case CaffeineTake::CaffeineState::Inactive: return lang->Tip_AutoInactive;
    case CaffeineTake::CaffeineState::Active:   return lang->Tip_AutoActive;
    }

    return L"";
}

auto AutoMode::IsModeAvailable () const -> bool
{
#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE)
    return true;
#else
    return false;
#endif
}

auto AutoMode::GetName () const -> std::wstring_view
{
    return L"Auto";
}

} // namespace CaffeineTake
