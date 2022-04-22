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
