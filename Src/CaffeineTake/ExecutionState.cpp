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
