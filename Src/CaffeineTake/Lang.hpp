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

#if defined(FEATURE_CAFFEINETAKE_MULTILANG)
#   include "Utility.hpp"
#   include <nlohmann/json.hpp>
#endif

#include <memory>
#include <string>

namespace CaffeineTake {

struct Lang;
using LangPtr = std::shared_ptr<Lang>;

struct Lang
{
    // Those two are set manually.
    std::wstring LangId   = L"en";
    std::wstring LangName = L"English";

    std::wstring ContextMenu_DisableCaffeine = L"Disable Caffeine";
    std::wstring ContextMenu_EnableCaffeine  = L"Enable Caffeine";
    std::wstring ContextMenu_EnableAuto      = L"Enable Caffeine (Auto)";
    std::wstring ContextMenu_EnableTimer     = L"Enable Caffeine (Timer)";
    std::wstring ContextMenu_Settings        = L"Settings";
    std::wstring ContextMenu_About           = L"About";
    std::wstring ContextMenu_Exit            = L"Exit";

    std::wstring Tip_DisabledInactive        = L"Caffeine - Off";
    std::wstring Tip_DisabledActive          = L"Caffeine - Off";
    std::wstring Tip_EnabledInactive         = L"Caffeine - On";
    std::wstring Tip_EnabledActive           = L"Caffeine - On";
    std::wstring Tip_AutoInactive            = L"Caffeine - Auto (Inactive)";
    std::wstring Tip_AutoActive              = L"Caffeine - Auto (Active)";
    std::wstring Tip_TimerInactive           = L"Caffeine - Timer (Inactive)";
    std::wstring Tip_TimerActive             = L"Caffeine - Timer (Active)";
};
    
#if defined(FEATURE_CAFFEINETAKE_MULTILANG)

inline auto to_json (nlohmann::json& j, const Lang& lang)
{
}

#define LOAD_LANG_STR(_str) lang.##_str = j.value<std::wstring>(#_str, def.##_str)

inline auto from_json (const nlohmann::json& j, Lang& lang)
{
    static auto def = Lang();

    LOAD_LANG_STR(ContextMenu_DisableCaffeine);
    LOAD_LANG_STR(ContextMenu_EnableCaffeine );
    LOAD_LANG_STR(ContextMenu_EnableAuto     );
    LOAD_LANG_STR(ContextMenu_EnableTimer    );
    LOAD_LANG_STR(ContextMenu_Settings       );
    LOAD_LANG_STR(ContextMenu_About          );
    LOAD_LANG_STR(ContextMenu_Exit           );

    LOAD_LANG_STR(Tip_DisabledInactive       );
    LOAD_LANG_STR(Tip_DisabledActive         );
    LOAD_LANG_STR(Tip_EnabledInactive        );
    LOAD_LANG_STR(Tip_EnabledActive          );
    LOAD_LANG_STR(Tip_AutoInactive           );
    LOAD_LANG_STR(Tip_AutoActive             );
    LOAD_LANG_STR(Tip_TimerInactive          );
    LOAD_LANG_STR(Tip_TimerActive            );
}

#endif

} // namespace CaffeineTake
