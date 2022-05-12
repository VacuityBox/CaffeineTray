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
#include "Logger.hpp"
#include "Scanner.hpp"
#include "Schedule.hpp"
#include "Settings.hpp"
#include "ThreadTimer.hpp"

#include <string_view>

namespace CaffeineTake {

enum class CaffeineMode : unsigned char
{
    Disabled = 0,
    Standard = 1, // aka Enabled, aka Active
    Auto     = 2,
    Timer    = 3
};

constexpr auto CaffeineModeToString (CaffeineMode mode) -> std::wstring_view
{
    switch (mode)
    {
    case CaffeineMode::Disabled: return L"Disabled";
    case CaffeineMode::Standard: return L"Standard";
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

    virtual auto IsModeAvailable () -> bool = 0;

    //virtual auto OnCustomMessage (UINT, WPARAM, LPARAM) -> bool = 0;
};

class DisabledMode : public Mode
{
public:
    DisabledMode (CaffeineAppSO app);

    auto Start () -> bool override;
    auto Stop () -> bool override;

    auto IsModeAvailable () -> bool override;
};

class StandardMode : public Mode
{
public:
    StandardMode (CaffeineAppSO app);

    auto Start () -> bool override;
    auto Stop () -> bool override;

    auto IsModeAvailable () -> bool override;
};

class AutoMode : public Mode
{
    ProcessScanner   mProcessScanner;
    WindowScanner    mWindowScanner;
    UsbDeviceScanner mUsbScanner;
    BluetoothScanner mBluetoothScanner;

    bool             mScannerPreviousResult = false;
    ThreadTimer      mScannerTimer;

    auto ScannerTimerProc  (const StopToken& stop, const PauseToken& pause) -> bool;

    bool             mSchedulePreviousResult = false;
    ThreadTimer      mScheduleTimer;

    auto ScheduleTimerProc (const StopToken& stop, const PauseToken& pause) -> bool;

public:
    AutoMode (CaffeineAppSO app);

    auto Start () -> bool override;
    auto Stop  () -> bool override;

    auto IsModeAvailable () -> bool override;
};

class TimerMode : public Mode
{
    ThreadTimer mTimerThread;

    auto TimerProc (const StopToken& stop, const PauseToken& pause) -> bool;

public:
    TimerMode (CaffeineAppSO app);

    auto Start () -> bool override;
    auto Stop () -> bool override;

    auto IsModeAvailable () -> bool override;
};

} // namespace CaffeineTake
