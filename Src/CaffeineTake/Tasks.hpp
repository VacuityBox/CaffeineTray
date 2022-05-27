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

#include <string_view>

namespace CaffeineTake {

struct Task
{
    std::wstring_view Name;
    unsigned int      MessageId;

    constexpr Task (const wchar_t* name, unsigned int id)
        : Name      (name)
        , MessageId (id)
    {
    }

    operator std::wstring_view () const { return Name; }
    operator unsigned int      () const { return MessageId; }

    static constexpr auto Invalid () -> Task
    {
        return Task(L"", 0);
    }
};

static constexpr auto TASK_DISBALE_CAFFEINE     = Task(L"/task:DisableCaffeine",    1);
static constexpr auto TASK_ENABLE_STANDARD_MODE = Task(L"/task:EnableStandardMode", 2);
static constexpr auto TASK_ENABLE_AUTO_MODE     = Task(L"/task:EnableAutoMode",     3);
static constexpr auto TASK_ENABLE_TIMER_MODE    = Task(L"/task:EnableTimerMode",    4);
static constexpr auto TASK_SHOW_SETTINGS_DIALOG = Task(L"/task:Settings",           5);
static constexpr auto TASK_SHOW_ABOUT_DIALOG    = Task(L"/task:About",              6);
static constexpr auto TASK_EXIT                 = Task(L"/task:Exit",               7);

} // namespace CaffeineTake
