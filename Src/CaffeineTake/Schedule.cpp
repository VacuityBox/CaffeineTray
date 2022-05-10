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

#include "Config.hpp"
#include "Schedule.hpp"

#include "Logger.hpp"
#include "Utility.hpp"

#include <chrono>
#include <format>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace CaffeineTake {

auto Schedule::CheckSchedule (
    const std::vector<ScheduleEntry>&                  schedule,
    std::chrono::time_point<std::chrono::system_clock> time
) -> bool {
#if !defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE)
    return false;
#else
    static auto s_IsInSchedule = false;
        
    const auto tz = std::chrono::current_zone();
    const auto localTime = tz->to_local(time);

    const auto hh  = std::stoul(std::format("{:%H}", localTime));
    const auto mm  = std::stoul(std::format("{:%M}", localTime));
    const auto ss  = std::stoul(std::format("{:%S}", localTime));
    const auto day = std::stoul(std::format("{:%u}", localTime));

    // Convert day to DaysOfWeek.
    const auto timeDayOfWeek = [&](){
        switch (day)
        {
        case 0: return DaysOfWeek::Sunday;
        case 1: return DaysOfWeek::Monday;
        case 2: return DaysOfWeek::Tuesday;
        case 3: return DaysOfWeek::Wednesday;
        case 4: return DaysOfWeek::Thursday;
        case 5: return DaysOfWeek::Friday;
        case 6: return DaysOfWeek::Saturday;
        case 7: return DaysOfWeek::Sunday;
        }
            
        return DaysOfWeek::Monday;
    }();

    const auto timeSeconds = (hh * 3600) + (mm * 60) + ss;

    for (const auto& entry : schedule)
    {
        // Check if day match.
        if ((entry.ActiveDays & timeDayOfWeek) == timeDayOfWeek)
        {
            // Check if time match.
            for (const auto& tr : entry.ActiveHours)
            {
                if (tr.Begin <= timeSeconds && timeSeconds <= tr.End)
                {
                    if (!s_IsInSchedule)
                    {
                        const auto fmt = std::format(
                            "Time is in schedule, {} in {}:[{}, {}]",
                            localTime,
                            static_cast<unsigned int>(timeDayOfWeek),
                            tr.Begin,
                            tr.End
                        );
                        LOG_INFO("{}", fmt);
                    }
                    s_IsInSchedule = true;
                    return true;
                }
            }
        }
    }

    s_IsInSchedule = false;

    return false;
#endif
}

} // namespace CaffeineTake
