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
#include "CommandLineArgs.hpp"

namespace CaffeineTake {

auto CheckArg (const std::wstring_view text, CommandLineArgs& args) -> void
{
    if (text.empty())
    {
        return;
    }

    if      (text == TASK_DISBALE_CAFFEINE)     { args.Task = TASK_DISBALE_CAFFEINE; }
    else if (text == TASK_ENABLE_STANDARD_MODE) { args.Task = TASK_ENABLE_STANDARD_MODE; }
    else if (text == TASK_ENABLE_AUTO_MODE)     { args.Task = TASK_ENABLE_AUTO_MODE; }
    else if (text == TASK_ENABLE_TIMER_MODE)    { args.Task = TASK_ENABLE_TIMER_MODE; }
    else if (text == TASK_SHOW_SETTINGS_DIALOG) { args.Task = TASK_SHOW_SETTINGS_DIALOG; }
    else if (text == TASK_SHOW_ABOUT_DIALOG)    { args.Task = TASK_SHOW_ABOUT_DIALOG; }
    else if (text == TASK_SHOW_SETTINGS_DIALOG) { args.Task = TASK_SHOW_SETTINGS_DIALOG; }
    else if (text == TASK_EXIT)                 { args.Task = TASK_EXIT; }
}

auto ParseCommandLine (const std::wstring_view cmdline) -> CommandLineArgs
{
    auto args = CommandLineArgs();

    auto last = 0;
    for (auto i = 0;  i < cmdline.size(); ++i)
    {
        if (cmdline.at(i) == L' ')
        {
            auto text = std::wstring_view(cmdline.data(), i - last);
            last = i + 1;

            CheckArg(text, args);
        }
    }

    if (auto count = cmdline.size() - last; count > 0)
    {
        auto text = std::wstring_view(cmdline.data(), count);
        CheckArg(text, args);
    }

    return args;
}

} // namespace CaffeineTake
