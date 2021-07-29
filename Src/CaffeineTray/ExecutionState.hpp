#pragma once

namespace Caffeine {

class ExecutionState final
{
    bool mIsActive;
    bool mKeepDisplayOn;

    ExecutionState            (const ExecutionState&) = delete;
    ExecutionState& operator= (const ExecutionState&) = delete;

public:
    ExecutionState ()
        : mIsActive      (false)
        , mKeepDisplayOn (false)
    {
    }
    
    ~ExecutionState ()
    {
    }

    auto PreventSleep (bool keepDisplayOn) -> bool;
    auto AllowSleep   ()                   -> bool;

    auto IsActive () -> bool { return mIsActive; }
};

} // namespace Caffeine
