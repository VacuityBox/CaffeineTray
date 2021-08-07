#pragma once

#include "Logger.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>

namespace Caffeine {

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

    std::shared_ptr<Logger> mLogger;

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
    ExecutionState  (std::shared_ptr<Logger> logger);
    ~ExecutionState ();

    auto SystemRequired (bool keepDisplayOn) -> bool;
    auto Continuous     ()                   -> bool;

    auto IsSystemRequired () -> bool
    {
        auto guard = std::lock_guard(mRequestMutex);
        return mSystemRequired;
    }
};

} // namespace Caffeine
