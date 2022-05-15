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
#include "CaffeineSounds.hpp"
#include "Schedule.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

class Settings final
{
public:
    struct General
    {
        std::wstring              LangId                = L"en";
        CaffeineIcons::IconPack   IconPack              = CaffeineIcons::IconPack::Original;
        bool                      UseNotifyIcon         = true;
        bool                      UseJumpLists          = false;
        bool                      UseDockMode           = false;
        bool                      AutoStart             = false;
        bool                      ShowNotifications     = false;
        bool                      PlayNotificationSound = true;
        CaffeineSounds::SoundPack SoundPack             = CaffeineSounds::SoundPack::System;

        General () = default;
    } General;
    
    struct Standard
    {
        bool Enabled            = true;
        bool KeepScreenOn       = true;
        bool WhenSessionLocked  = false;

        Standard () = default;
    } Standard;

    struct Auto
    {
        bool         Enabled            = true;
        bool         KeepScreenOn       = true;
        bool         WhenSessionLocked  = false;
        unsigned int ScanInterval       = 2000;  // in ms

        struct TriggerProcess
        {
            bool                             Enabled          = true; 
            std::vector<std::wstring>        Processes        = std::vector<std::wstring>();
        } TriggerProcess;

        struct TriggerWindow
        {
            bool                             Enabled          = true; 
            std::vector<std::wstring>        Windows          = std::vector<std::wstring>();
        } TriggerWindow;
        
        struct TriggerUsb
        {
            bool                             Enabled          = true;
            std::vector<std::wstring>        UsbDevices       = std::vector<std::wstring>();
        } TriggerUsb;
        
        struct TriggerBluetooth
        {
            bool                             Enabled          = true;
            std::vector<BluetoothIdentifier> BluetoothDevices = std::vector<BluetoothIdentifier>();
            unsigned int                     ActiveTimeout    = 60*1000;   // in ms
        } TriggerBluetooth;

        struct TriggerSchedule
        {
            bool                             Enabled          = true;
            std::vector<ScheduleEntry>       ScheduleEntries  = std::vector<ScheduleEntry>();
        } TriggerSchedule;

        Auto () = default;
    } Auto;

    struct Timer
    {
        bool         Enabled           = true;
        bool         KeepScreenOn      = true;
        bool         WhenSessionLocked = false;
        unsigned int Interval          = 60*1000;

        Timer () = default;
    } Timer;

    Settings () = default;

    auto Load (const fs::path& path) -> bool;
    auto Save (const fs::path& path) -> bool;
};

} // namespace CaffeineTake
