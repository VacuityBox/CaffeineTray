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
#include "CaffeineIcons.hpp"
#include "Schedule.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

class Settings;
using SettingsPtr = std::shared_ptr<Settings>;

class Settings final
{
public:
    struct General
    {
        CaffeineIcons::IconPack IconPack;
        std::wstring            LangId;

        General ()
            : IconPack (CaffeineIcons::IconPack::Original)
            , LangId   (L"en")
        {
        }
    } General;
    
    struct Standard
    {
        bool KeepDisplayOn;
        bool DisableOnLockScreen;  // TODO rename to more adequate name, this one might be mistaken with disabling Caffeine at all

        Standard ()
            : KeepDisplayOn       (true)
            , DisableOnLockScreen (true)
        {
        }
    } Standard;

    struct Auto
    {
        bool KeepDisplayOn;
        bool DisableOnLockScreen; // TODO rename to more adequate name, this one might be mistaken with disabling Caffeine at all

        // Scan interval for Process/Window triggers.
        // TODO rename?
        unsigned int ScanInterval;  // in ms

        // Process trigger.
        std::vector<std::wstring> ProcessNames;
        std::vector<std::wstring> ProcessPaths;

        // Window trigger.
        std::vector<std::wstring> WindowTitles;

        // Schedule trigger.
        std::vector<ScheduleEntry> ScheduleEntries;

        // USB Device trigger.
        std::vector<std::wstring> UsbDevices;

        // Bluetooth Device trigger.
        std::vector<BluetoothIdentifier> BluetoothDevices;
        unsigned int                     ActiveTimeout;   // in ms

        Auto ()
            : KeepDisplayOn       (true)
            , DisableOnLockScreen (true)
            , ScanInterval        (2000)
            , ActiveTimeout       (60*1000)
        {
        }
    } Auto;

    struct Timer
    {
        bool KeepDisplayOn;
        bool DisableOnLockScreen;  // TODO rename to more adequate name, this one might be mistaken with disabling Caffeine at all

        unsigned int Interval;

        Timer ()
            : KeepDisplayOn       (true)
            , DisableOnLockScreen (true)
            , Interval            (0)
        {
        }
    } Timer;

    Settings () = default;

    auto Load (const fs::path& path) -> bool;
    auto Save (const fs::path& path) -> bool;
};

} // namespace CaffeineTake
