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

#include "CaffeineIcons.hpp"
#include "Schedule.hpp"
#include "Utility.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace CaffeineTake {

class Settings;
using SettingsPtr = std::shared_ptr<Settings>;

class Settings final
{
public:
    CaffeineIcons::IconPack IconPack;
    
    struct Standard
    {
        bool KeepDisplayOn;
        bool DisableOnLockScreen;  // TODO rename to more adequate name, this one might be mistaken with disabling Caffeine at all

        Standard ()
            : KeepDisplayOn       (true)
            , DisableOnLockScreen (true)
        {
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Standard, KeepDisplayOn, DisableOnLockScreen)
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

        Auto ()
            : KeepDisplayOn       (true)
            , DisableOnLockScreen (true)
            , ScanInterval        (2000)
        {
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
            Auto,
            KeepDisplayOn,
            DisableOnLockScreen,
            ScanInterval,
            ProcessNames,
            ProcessPaths,
            WindowTitles,
            ScheduleEntries,
            UsbDevices
        )
    } Auto;

    Settings ()
        : IconPack (CaffeineIcons::IconPack::Original)
    {
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Settings, IconPack, Standard, Auto)
};

} // namespace CaffeineTake
