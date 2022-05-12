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

#include "BluetoothIdentifier.hpp"
#include "ForwardDeclaration.hpp"
#include "ThreadTimer.hpp"
#include "Utility.hpp"

#include <chrono>
#include <map>
#include <string>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

class Scanner
{
public:
    virtual ~Scanner() {}

    virtual auto Run (SettingsPtr, const StopToken&, const PauseToken&) -> bool = 0;
};

class ProcessScanner : public Scanner
{
    std::wstring mLastProcessName = L"";
    std::wstring mLastProcessPath = L"";
    DWORD        mLastPid         = 0;

    auto CheckLast () -> bool;

public:
    auto Run (SettingsPtr settings, const StopToken& stop, const PauseToken& pause) -> bool override;
};

class WindowScanner : public Scanner
{
public:
    auto Run (SettingsPtr settings, const StopToken& stop, const PauseToken& pause) -> bool override;
};

class UsbDeviceScanner : public Scanner
{
    std::wstring mLastFoundDevice = L"";

public:
    auto Run (SettingsPtr settings, const StopToken& stop, const PauseToken& pause) -> bool override;
};

class BluetoothScanner : public Scanner
{
    using LocalTime   = std::chrono::local_time<std::chrono::system_clock::duration>;
    using LastSeenMap = std::map<unsigned long long, LocalTime>;

    BluetoothIdentifier  mLastFoundDevice  = BluetoothIdentifier();
    HMODULE              mLibBluetoothApis = NULL;
    LastSeenMap          mLastSeenMap      = LastSeenMap();
    LocalTime            mLastInquiryTime  = LocalTime();
    std::chrono::seconds mInquiryTimeout   = std::chrono::seconds(60);

    auto SystemTimeToChronoLocalTimePoint (const SYSTEMTIME& st);

    auto ShouldPerformDeviceInquiry   (const LocalTime& localTime, const std::chrono::seconds deviceActiveTimeout) -> bool;
    auto IssueDeviceInquiry           () -> bool;
    auto CheckIfThereIsBluetoothRadio () -> bool;

    auto EnumerateBluetoothDevices (
        SettingsPtr                settings,
        const LocalTime&           localTime,
        const std::chrono::seconds deviceActiveTimeout,
        const StopToken&           stop
    ) -> BluetoothIdentifier;

public:
    ~BluetoothScanner ()
    {
        if (mLibBluetoothApis)
        {
            FreeLibrary(mLibBluetoothApis);
        }
    }

    auto Run (SettingsPtr settings, const StopToken& stop, const PauseToken& pause) -> bool override;
};

} // namespace CaffeineTake
