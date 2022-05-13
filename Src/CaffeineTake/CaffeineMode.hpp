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
#include "CaffeineState.hpp"
#include "ForwardDeclaration.hpp"
#include "Scanner.hpp"
#include "Schedule.hpp"
#include "ThreadTimer.hpp"

#include <atomic>
#include <mutex>
#include <string_view>

namespace CaffeineTake {

enum class CaffeineMode : unsigned char
{
    Disabled = 0,
    Standard = 1, // aka Enabled, aka Active
    Auto     = 2,
    Timer    = 3
};

class Mode
{
protected:
    CaffeineAppSO mAppSO;

public:
    Mode (CaffeineAppSO app)
        : mAppSO (app)
    {
    }
    virtual ~Mode() = default;

    virtual auto Start () -> bool = 0;
    virtual auto Stop  () -> bool = 0;

    virtual auto GetIcon (CaffeineState state) const -> const HICON = 0;
    virtual auto GetTip  (CaffeineState state) const -> const std::wstring& = 0;

    virtual auto IsModeAvailable () const -> bool = 0;
    virtual auto GetName         () const -> std::wstring_view = 0;

    //virtual auto OnCustomMessage (UINT, WPARAM, LPARAM) -> bool = 0;
};

class DisabledMode : public Mode
{
public:
    DisabledMode (CaffeineAppSO app);

    auto Start () -> bool override;
    auto Stop  () -> bool override;

    auto GetIcon (CaffeineState state) const -> const HICON override;
    auto GetTip  (CaffeineState state) const -> const std::wstring& override;

    auto IsModeAvailable () const -> bool override;
    auto GetName         () const -> std::wstring_view override;
};

class StandardMode : public Mode
{
public:
    StandardMode (CaffeineAppSO app);

    auto Start () -> bool override;
    auto Stop () -> bool override;

    auto GetIcon (CaffeineState state) const -> const HICON override;
    auto GetTip  (CaffeineState state) const -> const std::wstring& override;

    auto IsModeAvailable () const -> bool override;
    auto GetName         () const -> std::wstring_view override;
};

class AutoMode : public Mode
{
    std::mutex         mScanMutex;
    std::atomic<bool>  mScannerResult;
    std::atomic<bool>  mScheduleResult;

    ProcessScanner     mProcessScanner;
    WindowScanner      mWindowScanner;
    UsbDeviceScanner   mUsbScanner;
    BluetoothScanner   mBluetoothScanner;

    ThreadTimer        mScannerTimer;
    ThreadTimer        mScheduleTimer;

    auto ScannerTimerProc  (const StopToken& stop, const PauseToken& pause) -> bool;
    auto ScheduleTimerProc (const StopToken& stop, const PauseToken& pause) -> bool;

public:
    AutoMode (CaffeineAppSO app);

    auto Start () -> bool override;
    auto Stop  () -> bool override;

    auto GetIcon (CaffeineState state) const -> const HICON override;
    auto GetTip  (CaffeineState state) const -> const std::wstring& override;

    auto IsModeAvailable () const -> bool override;
    auto GetName         () const -> std::wstring_view override;
};

class TimerMode : public Mode
{
    ThreadTimer mTimerThread;

    auto TimerProc (const StopToken& stop, const PauseToken& pause) -> bool;

public:
    TimerMode (CaffeineAppSO app);

    auto Start () -> bool override;
    auto Stop  () -> bool override;

    auto GetIcon (CaffeineState state) const -> const HICON override;
    auto GetTip  (CaffeineState state) const -> const std::wstring& override;

    auto IsModeAvailable () const -> bool override;
    auto GetName         () const -> std::wstring_view override;
};

} // namespace CaffeineTake
