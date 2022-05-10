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

#include <chrono>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace CaffeineTake {

enum class DaysOfWeek : unsigned int
{
    Monday    = 0x01,
    Tuesday   = 0x02,
    Wednesday = 0x04,
    Thursday  = 0x08,
    Friday    = 0x10,
    Saturday  = 0x20,
    Sunday    = 0x40
};
DEFINE_ENUM_FLAG_OPERATORS(DaysOfWeek);

// In seconds. 0 == 0:00:00, 84600-1 == 23:59:59, end > begin
struct TimeRange
{
    unsigned int Begin;
    unsigned int End;
};

using TimeRangeList = std::vector<TimeRange>;

struct ScheduleEntry
{
    std::wstring  Name;         // optional
    DaysOfWeek    ActiveDays;
    TimeRangeList ActiveHours;
};

class Schedule
{
public:
    static auto CheckSchedule (
        const std::vector<ScheduleEntry>&                  schedule,
        std::chrono::time_point<std::chrono::system_clock> time
    ) -> bool;
};

} // namespace CaffeineTake
