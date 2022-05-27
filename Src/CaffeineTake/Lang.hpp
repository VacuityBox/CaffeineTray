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

#include <filesystem>
#include <memory>
#include <string>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

class Lang final
{
public:
    // Those two are set manually.
    std::wstring LangId   = L"en";
    std::wstring LangName = L"English";

    std::wstring ContextMenu_DisableCaffeine = L"Disable Caffeine";
    std::wstring ContextMenu_EnableStandard  = L"Enable Caffeine (Standard)";
    std::wstring ContextMenu_EnableAuto      = L"Enable Caffeine (Auto)";
    std::wstring ContextMenu_EnableTimer     = L"Enable Caffeine (Timer)";
    std::wstring ContextMenu_Settings        = L"Settings";
    std::wstring ContextMenu_About           = L"About";
    std::wstring ContextMenu_Exit            = L"Exit";

    std::wstring Tip_DisabledInactive        = L"Caffeine - Disabled";
    std::wstring Tip_DisabledActive          = L"Caffeine - Disabled";
    std::wstring Tip_StandardInactive        = L"Caffeine - Standard (Inactive)";
    std::wstring Tip_StandardActive          = L"Caffeine - Standard (Active)";
    std::wstring Tip_AutoInactive            = L"Caffeine - Auto (Inactive)";
    std::wstring Tip_AutoActive              = L"Caffeine - Auto (Active)";
    std::wstring Tip_TimerInactive           = L"Caffeine - Timer (Inactive)";
    std::wstring Tip_TimerActive             = L"Caffeine - Timer (Active)";

    std::wstring Task_DisableCaffeine        = L"Disable Caffeine";
    std::wstring Task_EnableStandardMode     = L"Enable Standard Mode";
    std::wstring Task_EnableAutoMode         = L"Enable Auto Mode";
    std::wstring Task_EnableTimerMode        = L"Enable Timer Mode";
    std::wstring Task_Settings               = L"Settings";
    std::wstring Task_About                  = L"About";
    std::wstring Task_Exit                   = L"Exit";          

    auto Load (const fs::path& path) -> bool;
    auto Save (const fs::path& path) -> bool;
};

} // namespace CaffeineTake
