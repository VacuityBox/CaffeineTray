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

#include "PCH.hpp"
#include "Config.hpp"
#include "Lang.hpp"

#if defined(FEATURE_CAFFEINETAKE_MULTILANG)
#   include "Logger.hpp"
#   include "Serializers.hpp"
#   include <nlohmann/json.hpp>
#   include <fstream>
#endif

namespace CaffeineTake {

#if defined(FEATURE_CAFFEINETAKE_MULTILANG)

inline auto to_json (nlohmann::json& j, const Lang& lang)
{
}

#define LOAD_LANG_STR(_str) lang.##_str = j.value<std::wstring>(#_str, def.##_str)

inline auto from_json (const nlohmann::json& j, Lang& lang)
{
    static auto def = Lang();

    LOAD_LANG_STR(ContextMenu_DisableCaffeine);
    LOAD_LANG_STR(ContextMenu_EnableStandard );
    LOAD_LANG_STR(ContextMenu_EnableAuto     );
    LOAD_LANG_STR(ContextMenu_EnableTimer    );
    LOAD_LANG_STR(ContextMenu_Settings       );
    LOAD_LANG_STR(ContextMenu_About          );
    LOAD_LANG_STR(ContextMenu_Exit           );

    LOAD_LANG_STR(Tip_DisabledInactive       );
    LOAD_LANG_STR(Tip_DisabledActive         );
    LOAD_LANG_STR(Tip_StandardInactive       );
    LOAD_LANG_STR(Tip_StandardActive         );
    LOAD_LANG_STR(Tip_AutoInactive           );
    LOAD_LANG_STR(Tip_AutoActive             );
    LOAD_LANG_STR(Tip_TimerInactive          );
    LOAD_LANG_STR(Tip_TimerActive            );

    LOAD_LANG_STR(Task_DisableCaffeine       );
    LOAD_LANG_STR(Task_EnableStandardMode    );
    LOAD_LANG_STR(Task_EnableAutoMode        );
    LOAD_LANG_STR(Task_EnableTimerMode       );
    LOAD_LANG_STR(Task_Settings              );
    LOAD_LANG_STR(Task_About                 );
    LOAD_LANG_STR(Task_Exit                  );
}

#undef LOAD_LANG_STR

#endif

auto Lang::Load (const fs::path& path) -> bool
{
#if defined(FEATURE_CAFFEINETAKE_MULTILANG)
    // NOTE: Language file should be in UTF-8
    // Open lang file for read.
    auto file = std::ifstream(path);
    if (!file)
    {
        LOG_ERROR(L"Failed to open lang file '{}' for reading", path.wstring());
        return false;
    }

    // Deserialize.
    auto json = nlohmann::json::parse(file, nullptr, false, true);
    if (json.is_discarded())
    {
        LOG_ERROR(L"Failed to parse lang '{}'", path.wstring());
        return false;
    }
    
    try
    {
        json.get_to<Lang>(*this);
    }
    catch (nlohmann::json::exception& e)
    {
        LOG_ERROR(L"Failed to deserialize language file '{}'", path.wstring());
        LOG_DEBUG("what() {}", e.what());
        return false;
    }

    LOG_DEBUG("{}", json.dump(4));
    LOG_INFO(L"Loaded language '{}'", path.wstring());

    return true;
#else
    return false;
#endif
}

auto Lang::Save (const fs::path& path) -> bool
{
#if defined(FEATURE_CAFFEINETAKE_MULTILANG)
    return false;
#else
    return true;
#endif
}

} // namespace CaffeineTake
