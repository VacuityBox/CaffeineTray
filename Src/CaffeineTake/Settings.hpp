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
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "CaffeineIcons.hpp"
#include "CaffeineMode.hpp"
#include "Utility.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

// std::wstring serializer/deserializer.
namespace nlohmann {

template <>
struct adl_serializer<std::wstring>
{
    static void to_json(json& j, const std::wstring& opt)
    {
        auto utf8 = CaffeineTake::UTF16ToUTF8(opt.c_str());
        j = utf8 ? utf8.value() : "";
    }

    static void from_json(const json& j, std::wstring& opt)
    {
        auto utf16 = CaffeineTake::UTF8ToUTF16(j.get<std::string>());
        opt = utf16 ? utf16.value() : L"";
    }
};

} // namespace nlohmann

namespace CaffeineTake {

class Settings final
{
public:
    CaffeineMode            Mode;
    CaffeineIcons::IconPack IconPack;
    
    struct Standard
    {
        bool KeepDisplayOn;
        bool DisableOnLockScreen;

        Standard ()
            : KeepDisplayOn       (true)
            , DisableOnLockScreen (true)
        {
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Standard, KeepDisplayOn, DisableOnLockScreen)
    } Standard;

    struct Auto
    {
        unsigned int ScanInterval;  // in ms
        bool         KeepDisplayOn;
        bool         DisableOnLockScreen;

        std::vector<std::wstring> ProcessNames;
        std::vector<std::wstring> ProcessPaths;
        std::vector<std::wstring> WindowTitles;

        Auto ()
            : ScanInterval        (2000)
            , KeepDisplayOn       (true)
            , DisableOnLockScreen (true)
        {
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
            Auto,
            ScanInterval,
            KeepDisplayOn,
            DisableOnLockScreen,
            ProcessNames,
            ProcessPaths,
            WindowTitles
        )
    } Auto;

    Settings ()
        : Mode     (CaffeineMode::Disabled)
        , IconPack (CaffeineIcons::IconPack::Original)
    {
    }

     NLOHMANN_DEFINE_TYPE_INTRUSIVE(Settings, Mode, IconPack, Standard, Auto)
};

} // namespace CaffeineTake
