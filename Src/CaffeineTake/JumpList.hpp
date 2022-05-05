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
#include <string>
#include <string_view>
#include <vector>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

struct JumpListEntry
{
    enum class Type : bool
    {
        Normal,
        Separator
    };

    enum Type         Type         = Type::Normal;
    std::wstring_view TaskName     = L"";
    std::wstring_view CmdArguments = L"";
    std::wstring_view IconPath     = L"";
    int               IconId       = 0;

    constexpr JumpListEntry (
        enum Type         type,
        std::wstring_view task,
        std::wstring_view arguments,
        std::wstring_view iconPath,
        int               iconId
    )
        : Type         (type)
        , TaskName     (task)
        , CmdArguments (arguments)
        , IconPath     (iconPath)
        , IconId       (iconId)
    {
    }
};

class JumpList
{
public:
    static auto Update (const fs::path& target, const std::vector<JumpListEntry>& jumpList) -> bool;
    static auto Clear  () -> bool;
};

} // namespace CaffeineTake
