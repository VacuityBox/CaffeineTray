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

#include "ExecutionState.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace CaffeineTake {

ExecutionState::ExecutionState ()
    : mSystemRequired (false)
    , mKeepDisplayOn  (false)
    , mWorkerThread   (&ExecutionState::Worker, this)
    , mIsDone         (false)
{
}

ExecutionState::~ExecutionState ()
{
    mIsDone = true;
    mWorkerConditionVar.notify_one(); // notify in case thread is waiting on condition
    mWorkerThread.join();
}

auto ExecutionState::SendRequest (bool systemRequired, bool keepDisplayOn) -> void
{
    mRequestMutex.lock();
    
    mRequest.systemRequired = systemRequired;
    mRequest.keepDisplayOn  = keepDisplayOn;
    mRequest.waiting        = true;

    mRequestMutex.unlock();
    mWorkerConditionVar.notify_one();
}

auto ExecutionState::ProcessRequest () -> void
{
    auto flags = ES_CONTINUOUS;
    if (mRequest.systemRequired)
    {
        flags |= ES_SYSTEM_REQUIRED;
    }
    if (mRequest.keepDisplayOn)
    {
        flags |= ES_DISPLAY_REQUIRED;
    }

    SetThreadExecutionState(flags);

    mSystemRequired = mRequest.systemRequired;
    mKeepDisplayOn  = mRequest.keepDisplayOn;

    mRequest.waiting = false;
}

auto ExecutionState::Worker () -> void
{
    while (!mIsDone)
    {
        auto queueLock = std::unique_lock<std::mutex>(mRequestMutex); // this locks
        mWorkerConditionVar.wait(queueLock, 
            [&]
            {
                return mRequest.waiting || mIsDone; // return false to continue wait
            }
        );

        if (mRequest.waiting)
        {
            ProcessRequest();
        }
    }
}

auto ExecutionState::SystemRequired (bool keepDisplayOn) -> bool
{
    SendRequest(true, keepDisplayOn);

    return true;
}

auto ExecutionState::Continuous () -> bool
{
    SendRequest(false, false);
    
    return true;
}

} // namespace CaffeineTake
