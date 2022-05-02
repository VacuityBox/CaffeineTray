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

#include <chrono>

namespace CaffeineTake {

auto AutoMode::ScannerTimerProc () -> bool
{
    // Scan processes and windows if no process found.
    auto scannerResult = mProcessScanner.Run(mSettingsPtr);
    if (!scannerResult)
    {
        scannerResult = mWindowScanner.Run(mSettingsPtr);
    }
    if (!scannerResult)
    {
        scannerResult = mUsbScanner.Run(mSettingsPtr);
    }
    if (!scannerResult)
    {
        scannerResult = mBluetoothScanner.Run(mSettingsPtr);
    }

    // Only if there is state change.
    if (scannerResult != mScannerPreviousResult)
    {
        // Activate auto mode if process is found. Deactivate otherwise.
        if (scannerResult)
        {
            mAppPtr->EnableCaffeine();
        }
        else
        {
            mAppPtr->DisableCaffeine();
        }

        mScannerPreviousResult = scannerResult;
    }

    return true;
}

auto AutoMode::ScheduleTimerProc () -> bool
{
    // Scan processes and windows if no process found.
    auto scheduleResult = Schedule::CheckSchedule(
        mSettingsPtr->Auto.ScheduleEntries, std::chrono::system_clock::now()
    );

    // Only if there is state change.
    if (scheduleResult != mSchedulePreviousResult)
    {
        if (scheduleResult)
        {
            mAppPtr->EnableCaffeine();
        }
        else
        {
            mAppPtr->DisableCaffeine();
        }

        mSchedulePreviousResult = scheduleResult;
    }

    // TODO it would be better if we first check schedule and the run scanner
    return true;
}

} // namespace CaffeineTake
