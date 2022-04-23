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

#include <condition_variable>
#include <mutex>
#include <thread>

namespace CaffeineTake {

class ExecutionState final
{
    struct Request
    {
        bool waiting;
        bool systemRequired;
        bool keepDisplayOn;

        Request ()
            : waiting        (false)
            , systemRequired (false)
            , keepDisplayOn  (false)
        {
        }
    };

    bool    mSystemRequired;
    bool    mKeepDisplayOn;
    Request mRequest;

    std::mutex              mRequestMutex;
    std::condition_variable mWorkerConditionVar;
    std::thread             mWorkerThread;
    bool                    mIsDone;

    auto SendRequest    (bool systemRequired, bool keepDisplayOn) -> void;
    auto ProcessRequest () -> void;

    auto Worker () -> void;

    ExecutionState            (const ExecutionState&) = delete;
    ExecutionState& operator= (const ExecutionState&) = delete;

public:
    ExecutionState  ();
    ~ExecutionState ();

    auto SystemRequired (bool keepDisplayOn) -> bool;
    auto Continuous     ()                   -> bool;

    auto IsSystemRequired () -> bool
    {
        auto guard = std::lock_guard(mRequestMutex);
        return mSystemRequired;
    }
};

} // namespace CaffeineTake
